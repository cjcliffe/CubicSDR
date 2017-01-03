// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"
#include <sstream>

class ModemKitGMSK : public ModemKitDigital {
public:
    unsigned int fdelay, sps;
    float ebf;
    
    gmskdem demodGMSK;
    std::vector<liquid_float_complex> inputBuffer;
};


class ModemGMSK : public ModemDigital {
public:
    ModemGMSK();
    ~ModemGMSK();

    std::string getName();
    
    static ModemBase *factory();
    
    int checkSampleRate(long long sampleRate, int audioSampleRate);
    int getDefaultSampleRate();

    ModemArgInfoList getSettings();
    void writeSetting(std::string setting, std::string value);
    std::string readSetting(std::string setting);
    
    ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    void disposeKit(ModemKit *kit);
    
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);

private:
    int _sps;    // samples per symbol
    int _fdelay;    // filter delay
    float _ebf;
};

