// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemCW.h"

// We are given a baseband segment BW (default 500Hz) wide which we want to
// offset by mBeepFrequency (default 650Hz). This yields a spectrum.
//
//            |   |....|....|
//            |   |....|....|
//            |   |....|....|
// -----------|---|----|----|--
//            0  150  650  1150
//
ModemCW::ModemCW()
  : ModemAnalogVC(),
    mBeepFrequency(650.0),
    mLO(nullptr),
    mToReal(nullptr)
{
  mLO = nco_crcf_create (LIQUID_NCO);
  mToReal = firhilbf_create (5,60.0f);
  useSignalOutput(true);
  ModemArgInfo offsetArg;
  offsetArg.key = "offset";
  offsetArg.name = "Frequency Offset";
  offsetArg.value = std::to_string(mBeepFrequency);
  offsetArg.units = "Hz";
  offsetArg.description = "Frequency Offset / Beep frequency";
  offsetArg.type  = ModemArgInfo::FLOAT;
  offsetArg.range = ModemRange (200.0,1000.0);
  mArgInfoList.push_back(offsetArg);
}

ModemCW::~ModemCW() {
  if (mLO)
    nco_crcf_destroy (mLO);
  if (mToReal)
    firhilbf_destroy (mToReal);
}

void ModemCW::writeSetting(std::string setting, std::string value)
{
  if (setting == "offset") {
    mBeepFrequency = std::stof(value);
    rebuildKit();
  }
  ModemAnalogVC::writeSetting (setting,value);
}

std::string ModemCW::readSetting(std::string setting)
{
  if (setting == "offset") {
    return std::to_string(mBeepFrequency);
  }
  return ModemAnalogVC::readSetting (setting);
}

// ModemSettings ModemCW::readSettings ()
// {
//   ModemSettings s;
//   s["offset"] = std::to_string(mBeepFrequency);
//   s["auto"] = (mAutoGain)? "on" : "off";
//   s["gain"] = std::to_string(mGain);
//   return s;
// }

ModemBase *ModemCW::factory() {
    return new ModemCW;
}

std::string ModemCW::getName() {
    return "CW";
}

int ModemCW::checkSampleRate (long long srate, int arate)
{
  if (srate < MIN_BANDWIDTH)
    return MIN_BANDWIDTH;
  return srate;
}

int ModemCW::getDefaultSampleRate() {
  return MIN_BANDWIDTH;
}

// The modem object is asked to make a "ModemKit" given the IQ sample rate
// and the audio sample rate. For the CW modem the IQ sample rate is small
// or narrow bandwidth. The demodulated sample rate must be fast enough to
// sample 200-1000Hz tones. If the IQ sample rate is less than 2000Hz then
// one doesn't have the bandwidth for these tones. So we need to interpolate
// the input IQ to audioOut, frequency shift, then pass the real part.
// Simple solution is just interpolate the IQ data to the audio sample rate.
ModemKit *ModemCW::buildKit (long long sampleRate, int audioSampleRate)
{
  ModemKitCW *kit = new ModemKitCW();
  float As = 60.0f;
  double ratio = double(audioSampleRate) / double(sampleRate);
  kit->sampleRate = sampleRate;
  kit->audioSampleRate = audioSampleRate;
  kit->audioResampleRatio = ratio;
  kit->mInputResampler = msresamp_cccf_create (ratio,As);
  return kit;
}

void ModemCW::disposeKit (ModemKit *kit)
{
  ModemKitCW *cwkit = (ModemKitCW*) kit;
  msresamp_cccf_destroy (cwkit->mInputResampler);
  delete kit;
}

void ModemCW::initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input)
{
    bufSize = input->data.size();

    if (!bufSize) {
        return;
    }

    double audio_resample_ratio = akit->audioResampleRatio;

    size_t audio_out_size = (size_t)ceil((double) (bufSize) * audio_resample_ratio) + 512;

    // Just make everything the audio out size
    if (mInput.size() != audio_out_size) {
        if (mInput.capacity() < audio_out_size) {
            mInput.reserve(audio_out_size);
        }
        mInput.resize(audio_out_size);
    }
}

void ModemCW::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
  unsigned int outSize;
    float lsb;
    liquid_float_complex sig;
    ModemKitCW *cwkit = (ModemKitCW *)kit;

    initOutputBuffers(cwkit, input);

    if (!bufSize) {
        return;
    }

    // Interpolate IQ samples to full audio band. We need to be able to
    // sample at 2 times the desired beep frequency.
    msresamp_cccf_execute (cwkit->mInputResampler,&input->data[0],bufSize,&mInput[0],&outSize);

    // Make the shoe fit.
    if (demodOutputData.size() != outSize) {
      demodOutputData.resize(outSize);
    }

    // Set the LO to the desired beep frequency.
    nco_crcf_set_frequency(mLO,2.0*M_PI*mBeepFrequency/kit->audioSampleRate);

    // Mix up from base band by beep frequency. Extract real part
    for (int i = 0; i < outSize; i++) {
      nco_crcf_mix_up (mLO,mInput[i],&sig);
      nco_crcf_step (mLO);
      firhilbf_c2r_execute (mToReal,sig,&lsb,&demodOutputData[i]);
    }

    applyGain (demodOutputData);

    audioOut->channels = 1;
    audioOut->sampleRate = cwkit->audioSampleRate;
    audioOut->data.assign(demodOutputData.begin(), demodOutputData.end());
}
