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
    modemcf demodDPSK;
    modemcf demodDPSK2;
    modemcf demodDPSK4;
    modemcf demodDPSK8;
    modemcf demodDPSK16;
    modemcf demodDPSK32;
    modemcf demodDPSK64;
    modemcf demodDPSK128;
    modemcf demodDPSK256;
};

