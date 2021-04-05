// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemDPSK : public ModemDigital {
public:
    ModemDPSK();
    ~ModemDPSK() override;

    std::string getName() override;
    
    static ModemBase *factory();
    
    ModemArgInfoList getSettings() override;
    void writeSetting(std::string setting, std::string value) override;
    std::string readSetting(std::string setting) override;
    
    void updateDemodulatorCons(int cons_in);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    int cons;
    modem demodDPSK;
    modem demodDPSK2;
    modem demodDPSK4;
    modem demodDPSK8;
    modem demodDPSK16;
    modem demodDPSK32;
    modem demodDPSK64;
    modem demodDPSK128;
    modem demodDPSK256;
};

