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
        : ModemAnalog(),
          mBeepFrequency(650.0),
          mGain(15.0),
          mAutoGain(true),
          mLO(nullptr),
          mToReal(nullptr) {
    mLO = nco_crcf_create(LIQUID_NCO);
    mToReal = firhilbf_create(5, 60.0f);
    useSignalOutput(true);
}

ModemCW::~ModemCW() {
    if (mLO)
        nco_crcf_destroy(mLO);
    if (mToReal)
        firhilbf_destroy(mToReal);
}

ModemArgInfoList ModemCW::getSettings() {
    ModemArgInfoList args;

    ModemArgInfo offsetArg;
    offsetArg.key = "offset";
    offsetArg.name = "Frequency Offset";
    offsetArg.value = std::to_string(mBeepFrequency);
    offsetArg.units = "Hz";
    offsetArg.description = "Frequency Offset / Beep frequency (200-1000Hz)";
    offsetArg.type = ModemArgInfo::Type::FLOAT;
    offsetArg.range = ModemRange(200.0, 1000.0);
    args.push_back(offsetArg);

    ModemArgInfo autoGain;
    autoGain.key = "auto";
    autoGain.name = "Auto Gain";
    autoGain.value = "on";
    autoGain.type = ModemArgInfo::Type::STRING;
    std::vector<std::string> autoOpts;
    autoOpts.push_back("on");
    autoOpts.push_back("off");
    autoGain.optionNames = autoOpts;
    autoGain.options = autoOpts;
    args.push_back(autoGain);

    ModemArgInfo gain;
    gain.key = "gain";
    gain.name = "Audio Gain";
    gain.value = "15";
    gain.units = "dB";
    gain.description = "Gain Setting (0-40dB)";
    gain.range = ModemRange(0.0, 40.0);
    gain.type = ModemArgInfo::Type::FLOAT;
    args.push_back(gain);
    return args;
}

void ModemCW::writeSetting(std::string setting, std::string value) {
    if (setting == "offset") {
        mBeepFrequency = std::stof(value);
        rebuildKit();
    } else if (setting == "auto") {
        mAutoGain = (value == "on");
    } else if (setting == "gain") {
        mGain = std::stof(value);
    }
}

std::string ModemCW::readSetting(std::string setting) {
    if (setting == "offset") {
        return std::to_string(mBeepFrequency);
    } else if (setting == "auto") {
        return (mAutoGain) ? "on" : "off";
    } else if (setting == "gain") {
        return std::to_string(mGain);
    }
    return "";
}

ModemBase *ModemCW::factory() {
    return new ModemCW;
}

std::string ModemCW::getName() {
    return "CW";
}

int ModemCW::checkSampleRate(long long srate, int /* arate */) {
    if (srate < MIN_BANDWIDTH)
        return MIN_BANDWIDTH;
    return (int)srate;
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
ModemKit *ModemCW::buildKit(long long sampleRate, int audioSampleRate) {
    auto *kit = new ModemKitCW();
    float As = 60.0f;
    double ratio = double(audioSampleRate) / double(sampleRate);
    kit->sampleRate = sampleRate;
    kit->audioSampleRate = audioSampleRate;
    kit->audioResampleRatio = ratio;
    kit->mInputResampler = msresamp_cccf_create((float)ratio, As);
    return kit;
}

void ModemCW::disposeKit(ModemKit *kit) {
    auto *cwkit = (ModemKitCW *) kit;
    msresamp_cccf_destroy(cwkit->mInputResampler);
    delete kit;
}

void ModemCW::initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input) {
    bufSize = input->data.size();

    if (!bufSize) {
        return;
    }

    double audio_resample_ratio = akit->audioResampleRatio;

    size_t audio_out_size = (size_t) ceil((double) (bufSize) * audio_resample_ratio) + 512;

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
    auto *cwkit = (ModemKitCW *) kit;

    initOutputBuffers(cwkit, input);

    if (!bufSize) {
        return;
    }

    // Interpolate IQ samples to full audio band. We need to be able to
    // sample at 2 times the desired beep frequency.
    msresamp_cccf_execute(cwkit->mInputResampler, &input->data[0], (unsigned int)bufSize, &mInput[0], &outSize);

    // Make the shoe fit.
    if (demodOutputData.size() != outSize) {
        demodOutputData.resize(outSize);
    }

    // Set the LO to the desired beep frequency.
    nco_crcf_set_frequency(mLO, 2.0f * (float)M_PI * mBeepFrequency / kit->audioSampleRate);

    // Mix up from base band by beep frequency. Extract real part
    for (unsigned int i = 0; i < outSize; i++) {
        nco_crcf_mix_up(mLO, mInput[i], &sig);
        nco_crcf_step(mLO);
        firhilbf_c2r_execute(mToReal, sig, &lsb, &demodOutputData[i]);
    }

    // Determine gain automagically (if desired)
    if (mAutoGain) {
        aOutputCeilMA = aOutputCeilMA + (aOutputCeil - aOutputCeilMA) * 0.025f;
        aOutputCeilMAA = aOutputCeilMAA + (aOutputCeilMA - aOutputCeilMAA) * 0.025f;
        aOutputCeil = 0;

        for (size_t i = 0; i < outSize; i++) {
            if (demodOutputData[i] > aOutputCeil) {
                aOutputCeil = demodOutputData[i];
            }
        }

        mGain = 10.0f * std::log10(0.5f / aOutputCeilMAA);
    }

    // Apply gain to demodulated output data
    for (size_t i = 0; i < outSize; i++) {
        demodOutputData[i] *= std::pow(10.0f, mGain / 10.0f);
    }

    audioOut->channels = 1;
    audioOut->sampleRate = cwkit->audioSampleRate;
    audioOut->data.assign(demodOutputData.begin(), demodOutputData.end());
}
