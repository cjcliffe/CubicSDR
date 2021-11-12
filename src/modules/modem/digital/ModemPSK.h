// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemPSK : public ModemDigital {
public:
    ModemPSK();
    ~ModemPSK() override;
    
    std::string getName() override;
    
    static ModemBase *factory();
    
    ModemArgInfoList getSettings() override;
    void writeSetting(std::string setting, std::string value) override;
    std::string readSetting(std::string setting) override;
    
    void updateDemodulatorCons(int cons_in);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    int cons;
    modemcf demodPSK;
    modemcf demodPSK2;
    modemcf demodPSK4;
    modemcf demodPSK8;
    modemcf demodPSK16;
    modemcf demodPSK32;
    modemcf demodPSK64;
    modemcf demodPSK128;
    modemcf demodPSK256;
};

