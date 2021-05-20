// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemAnalog.h"

class ModemUSB : public ModemAnalogVC {
public:
    ModemUSB();
    ~ModemUSB();

    std::string getName();

    static ModemBase *factory();

    int checkSampleRate(long long sampleRate, int audioSampleRate);
    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);

private:
	iirfilt_crcf ssbFilt;
	firhilbf c2rFilt;
    nco_crcf ssbShift;
};
