// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemAPSK : public ModemDigital {
public:
    ModemAPSK();
    ~ModemAPSK() override;

    std::string getName() override;
    
    static ModemBase *factory();

    ModemArgInfoList getSettings() override;
    void writeSetting(std::string setting, std::string value) override;
    std::string readSetting(std::string setting) override;

    void updateDemodulatorCons(int cons_in);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    int cons;
    modemcf demodAPSK;
    modemcf demodAPSK4;
    modemcf demodAPSK8;
    modemcf demodAPSK16;
    modemcf demodAPSK32;
    modemcf demodAPSK64;
    modemcf demodAPSK128;
    modemcf demodAPSK256;
};
