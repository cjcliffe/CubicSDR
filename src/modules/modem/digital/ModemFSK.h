// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"
#include <sstream>

class ModemKitFSK : public ModemKitDigital {
public:
    unsigned int m, k;
    float bw;
    
    fskdem demodFSK;
    std::vector<liquid_float_complex> inputBuffer;
};


class ModemFSK : public ModemDigital {
public:
    ModemFSK();
    ~ModemFSK() override;

    std::string getName() override;
    
    static ModemBase *factory();
    
    int checkSampleRate(long long sampleRate, int audioSampleRate) override;
    int getDefaultSampleRate() override;

    ModemArgInfoList getSettings() override;
    void writeSetting(std::string setting, std::string value) override;
    std::string readSetting(std::string setting) override;
    
    ModemKit *buildKit(long long sampleRate, int audioSampleRate) override;
    void disposeKit(ModemKit *kit) override;
    
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;

private:
    int sps, bps;
    float bw;
};

