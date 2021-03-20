// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "Modem.h"
#include "ModemAnalog.h"

class ModemKitCW : public ModemKitAnalog {
public:
    ModemKitCW() : ModemKitAnalog() {
    };
    msresamp_cccf mInputResampler;
};

class ModemCW : public ModemAnalog {
public:
    ModemCW();

    ~ModemCW();

    std::string getName();

    static ModemBase *factory();

    int checkSampleRate(long long srate, int arate);

    ModemKit *buildKit(long long srate, int arate);

    void disposeKit(ModemKit *kit);

    void initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input);

    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);

    ModemArgInfoList getSettings();

    void writeSetting(std::string setting, std::string value);

    std::string readSetting(std::string setting);

    // No resampling required.
    std::vector<float> *getResampledOutputData() { return &demodOutputData; }

private:
    float mBeepFrequency;
    float mGain;
    bool mAutoGain;
    nco_crcf mLO;
    firhilbf mToReal;
    std::vector<liquid_float_complex> mInput;
};
