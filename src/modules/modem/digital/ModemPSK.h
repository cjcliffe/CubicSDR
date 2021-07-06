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
    modem demodPSK;
    modem demodPSK2;
    modem demodPSK4;
    modem demodPSK8;
    modem demodPSK16;
    modem demodPSK32;
    modem demodPSK64;
    modem demodPSK128;
    modem demodPSK256;
};

