// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemAnalog.h"

ModemAnalog::ModemAnalog()
  : Modem()
{
}

ModemAnalogVC::ModemAnalogVC ()
  : ModemAnalog(),
    mAutoGain(false),
    mGain(15.0),
    aOutputCeil(1),
    aOutputCeilMA(1),
    aOutputCeilMAA(1)
{
  // Make volume controls
  ModemArgInfo autoGain;
  autoGain.key = "auto";
  autoGain.name = "Auto Gain";
  autoGain.value = "off";
  autoGain.type = ModemArgInfo::STRING;
  std::vector<std::string> autoOpts;
  autoOpts.push_back("on");
  autoOpts.push_back("off");
  autoGain.optionNames = autoOpts;
  autoGain.options = autoOpts;
  mArgInfoList.push_back(autoGain);

  ModemArgInfo gain;
  gain.key = "gain";
  gain.name = "Audio Gain";
  gain.value = "15";
  gain.units = "dB";
  gain.description = "Gain Setting";
  std::vector<std::string> gainOpts;
  for (int n = 0; n < 60; n += 5) {
    gainOpts.push_back(std::to_string(n));
  }
  gain.optionNames = gainOpts;
  gain.options = gainOpts;
  gain.type = ModemArgInfo::STRING;
  mArgInfoList.push_back(gain);
}

void ModemAnalogVC::writeSetting (std::string setting, std::string value)
{
  if (setting == "auto") {
    mAutoGain = (value=="on")?true:false;
  } else
  if (setting == "gain") {
    mGain = std::stof(value);
  }
}

std::string ModemAnalogVC::readSetting (std::string setting)
{
  if (setting == "auto") {
    return (mAutoGain)?"on":"off";
  } else
  if (setting == "gain") {
    return std::to_string(mGain);
  }
  return "";
}

std::string ModemAnalog::getType() {
    return "analog";
}

int ModemAnalog::checkSampleRate(long long sampleRate, int /* audioSampleRate */) {
    if (sampleRate < MIN_BANDWIDTH) {
        return MIN_BANDWIDTH;
    }
    return (int)sampleRate;
}

ModemKit *ModemAnalog::buildKit(long long sampleRate, int audioSampleRate) {
    auto *akit = new ModemKitAnalog;
    
    // stop-band attenuation [dB]
    float As = 60.0f;

    akit->sampleRate = sampleRate;
    akit->audioSampleRate = audioSampleRate;
    akit->audioResampleRatio = double(audioSampleRate) / double(sampleRate);
    akit->audioResampler = msresamp_rrrf_create((float)akit->audioResampleRatio, As);

    return akit;
}

void ModemAnalog::disposeKit(ModemKit *kit) {
    auto *akit = (ModemKitAnalog *)kit;
    
    msresamp_rrrf_destroy(akit->audioResampler);
    delete akit;
}

void ModemAnalog::initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input) {
    bufSize = input->data.size();

    if (!bufSize) {
        return;
    }

    double audio_resample_ratio = akit->audioResampleRatio;

    size_t audio_out_size = (size_t)ceil((double) (bufSize) * audio_resample_ratio) + 512;

    if (demodOutputData.size() != bufSize) {
        if (demodOutputData.capacity() < bufSize) {
            demodOutputData.reserve(bufSize);
        }
        demodOutputData.resize(bufSize);
    }
    if (resampledOutputData.size() != audio_out_size) {
        if (resampledOutputData.capacity() < audio_out_size) {
            resampledOutputData.reserve(audio_out_size);
        }
        resampledOutputData.resize(audio_out_size);
    }
}

void ModemAnalogVC::applyGain (std::vector<float> &adata)
{
  if (mAutoGain) {
      aOutputCeilMA = aOutputCeilMA + (aOutputCeil - aOutputCeilMA) * 0.025f;
      aOutputCeilMAA = aOutputCeilMAA + (aOutputCeilMA - aOutputCeilMAA) * 0.025f;
      aOutputCeil = 0;

      for (size_t i = 0; i < adata.size(); i++) {
          if (adata[i] > aOutputCeil) {
              aOutputCeil = adata[i];
          }
      }

      mGain = 10.0*std::log10(0.5f / aOutputCeilMAA);
  }

  // Apply gain to demodulated output data
  for (size_t i = 0; i < adata.size(); i++) {
    adata[i] *= std::pow(10.0,mGain/10.0);
  }
}

void ModemAnalog::buildAudioOutput(ModemKitAnalog *akit, AudioThreadInput *audioOut, bool autoGain) {
    unsigned int numAudioWritten;

    // if (autoGain) {
    //     aOutputCeilMA = aOutputCeilMA + (aOutputCeil - aOutputCeilMA) * 0.025f;
    //     aOutputCeilMAA = aOutputCeilMAA + (aOutputCeilMA - aOutputCeilMAA) * 0.025f;
    //     aOutputCeil = 0;
    //
    //     for (size_t i = 0; i < bufSize; i++) {
    //         if (demodOutputData[i] > aOutputCeil) {
    //             aOutputCeil = demodOutputData[i];
    //         }
    //     }
    //
    //     float gain = 0.5f / aOutputCeilMAA;
    //
    //     for (size_t i = 0; i < bufSize; i++) {
    //         demodOutputData[i] *= gain;
    //     }
    // }

    msresamp_rrrf_execute(akit->audioResampler, &demodOutputData[0], (int)demodOutputData.size(), &resampledOutputData[0], &numAudioWritten);

    audioOut->channels = 1;
    audioOut->sampleRate = akit->audioSampleRate;
    audioOut->data.assign(resampledOutputData.begin(), resampledOutputData.begin() + numAudioWritten);
}

std::vector<float> *ModemAnalog::getDemodOutputData() {
    return &demodOutputData;
}

std::vector<float> *ModemAnalog::getResampledOutputData() {
    return &resampledOutputData;
}
