// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemAPSK : public ModemDigital {
public:
    ModemAPSK();
    ~ModemAPSK();

    std::string getName();
    
    static ModemBase *factory();

    ModemArgInfoList getSettings();
    void writeSetting(std::string setting, std::string value);
    std::string readSetting(std::string setting);

    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    int cons;
    modem demodAPSK;
    modem demodAPSK4;
    modem demodAPSK8;
    modem demodAPSK16;
    modem demodAPSK32;
    modem demodAPSK64;
    modem demodAPSK128;
    modem demodAPSK256;
};
