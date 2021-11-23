// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemAnalog.h"

class ModemUSB : public ModemAnalogVC {
public:
    ModemUSB();
    ~ModemUSB() override;
    
    std::string getName() override;
    
    static ModemBase *factory();
    
    int checkSampleRate(long long sampleRate, int audioSampleRate) override;
    int getDefaultSampleRate() override;

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
	iirfilt_crcf ssbFilt;
	firhilbf c2rFilt;
    nco_crcf ssbShift;
};
