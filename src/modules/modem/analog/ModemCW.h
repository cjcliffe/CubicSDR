// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "Modem.h"
#include "ModemAnalog.h"

class ModemKitCW : public ModemKitAnalog {
public:
    ModemKitCW() : ModemKitAnalog() {
    };
    msresamp_cccf mInputResampler{};
};

class ModemCW : public ModemAnalog {
public:
    ModemCW();

    ~ModemCW() override;

    std::string getName() override;

    static ModemBase *factory();

    int checkSampleRate(long long srate, int arate) override;

    ModemKit *buildKit(long long srate, int arate) override;

    void disposeKit(ModemKit *kit) override;

    void initOutputBuffers(ModemKitAnalog *akit, ModemIQData *input) override;

    int getDefaultSampleRate() override;

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;

    ModemArgInfoList getSettings() override;

    void writeSetting(std::string setting, std::string value) override;

    std::string readSetting(std::string setting) override;

    // No resampling required.
    std::vector<float> *getResampledOutputData() override { return &demodOutputData; }

private:
    float mBeepFrequency;
    float mGain;
    bool mAutoGain;
    nco_crcf mLO;
    firhilbf mToReal;
    std::vector<liquid_float_complex> mInput;
};
