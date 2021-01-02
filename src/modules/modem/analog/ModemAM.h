// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemAM : public ModemAnalog {
public:
    ModemAM();
    ~ModemAM();

    std::string getName();

    static ModemBase *factory();

    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);

private:
    firfilt_rrrf mDCBlock;
};
