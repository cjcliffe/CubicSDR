// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemFM : public ModemAnalog {
public:
    ModemFM();
    ~ModemFM();
    
    std::string getName();
    
    static ModemBase *factory();

    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);

private:
    freqdem demodFM;
};