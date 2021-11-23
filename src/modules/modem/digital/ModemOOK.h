// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemOOK : public ModemDigital {
public:
    ModemOOK();
    ~ModemOOK() override;
    
    std::string getName() override;
    
    static ModemBase *factory();
    
    int checkSampleRate(long long sampleRate, int audioSampleRate) override;

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    modemcf demodOOK;
};
