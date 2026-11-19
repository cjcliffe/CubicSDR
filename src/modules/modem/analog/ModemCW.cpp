// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemCW.h"
#include "ResampleGuard.h"
#include <iostream>

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
          mLowerSide(false),
          mLO(nullptr),
          mToReal(nullptr) {
    mLO = nco_crcf_create(LIQUID_NCO);
    mToReal = firhilbf_create(5, 60.0f);
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

    ModemArgInfo side;
    side.key = "side";
    side.name = "Sideband";
    side.value = "upper";
    side.type = ModemArgInfo::Type::STRING;
    std::vector<std::string> sideOpts;
    sideOpts.push_back("upper");
    sideOpts.push_back("lower");
    side.optionNames = sideOpts;
    side.options = sideOpts;
    args.push_back(side);
    return args;
}

void ModemCW::writeSetting(std::string setting, std::string value) {
    if (setting == "offset") {
        mBeepFrequency = std::stof(value);
    } else if (setting == "side") {
        mLowerSide = (value == "lower");
    } else if (setting == "auto") {
        mAutoGain = (value == "on");
    } else if (setting == "gain") {
        mGain = std::stof(value);
    }
}

std::string ModemCW::readSetting(std::string setting) {
    if (setting == "offset") {
        return std::to_string(mBeepFrequency);
    } else if (setting == "side") {
        return mLowerSide ? "lower" : "upper";
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
    double ratio = clampResampleRatio(double(audioSampleRate) / double(sampleRate));
    kit->sampleRate = sampleRate;
    kit->audioSampleRate = audioSampleRate;
    // CW levels post-resample audio, so the follower runs at the audio rate.
    autoGainCtl.setSampleRate((double)audioSampleRate);
    kit->audioResampleRatio = ratio;
    kit->mInputResampler = msresamp_cccf_create((float)ratio, As);
    if (!kit->mInputResampler) {
        std::cout << "ModemCW: msresamp_create failed; audio muted." << std::endl << std::flush;
    }
    return kit;
}

void ModemCW::disposeKit(ModemKit *kit) {
    auto *cwkit = (ModemKitCW *) kit;
    if (cwkit->mInputResampler) {
        msresamp_cccf_destroy(cwkit->mInputResampler);
    }
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
    liquid_float_complex sig;
    auto *cwkit = (ModemKitCW *) kit;

    initOutputBuffers(cwkit, input);

    if (!bufSize) {
        return;
    }

    // Interpolate IQ samples to full audio band. We need to be able to
    // sample at 2 times the desired beep frequency.
    if (cwkit->mInputResampler) {
        msresamp_cccf_execute(cwkit->mInputResampler, &input->data[0], (unsigned int)bufSize, &mInput[0], &outSize);
    } else {
        outSize = 0;
    }

    // Make the shoe fit.
    if (demodOutputData.size() != outSize) {
        demodOutputData.resize(outSize);
    }

    // Set the LO to the desired beep frequency; lower sideband mirrors it.
    float beep = mLowerSide ? -mBeepFrequency : mBeepFrequency;
    nco_crcf_set_frequency(mLO, 2.0f * (float)M_PI * beep / kit->audioSampleRate);

    // Mix up from base band by beep frequency. Extract real part
    for (unsigned int i = 0; i < outSize; i++) {
        nco_crcf_mix_up(mLO, mInput[i], &sig);
        nco_crcf_step(mLO);
        float y0, y1;
        firhilbf_c2r_execute(mToReal, sig, &y0, &y1);
        demodOutputData[i] = mLowerSide ? y0 : y1;
    }

    // Determine gain automagically (if desired); the follower tracks the
    // beep tone level so keying transients do not desense it.
    if (mAutoGain) {
        toneLevel.configure(kit->audioSampleRate, mBeepFrequency);
        float tonePeak = toneLevel.measureBlock(demodOutputData.data(), outSize);
        float agcGain = autoGainCtl.processLevel(tonePeak, outSize);
        mGain = linearGainToDb(agcGain);
        // Auto may exceed the manual 0-40 range on weak signals.
        mGain = mGain < 0.0f ? 0.0f : (mGain > 40.0f ? 40.0f : mGain);
    }

    // Apply gain to demodulated output data
    float linGain = dbToLinearGain(mGain);
    for (size_t i = 0; i < outSize; i++) {
        demodOutputData[i] *= linGain;
    }

    audioOut->channels = 1;
    audioOut->sampleRate = cwkit->audioSampleRate;
    audioOut->data.assign(demodOutputData.begin(), demodOutputData.end());
}
