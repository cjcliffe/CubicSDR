// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemAM : public ModemAnalogVC {
public:
    ModemAM();
    ~ModemAM() override;

    std::string getName() override;

    static ModemBase *factory();

    int getDefaultSampleRate() override;

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;

private:
    firfilt_rrrf mDCBlock;
};
