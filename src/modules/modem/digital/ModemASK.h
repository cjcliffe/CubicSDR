// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemASK : public ModemDigital {
public:
    ModemASK();
    ~ModemASK() override;
    
    std::string getName() override;
    
    static ModemBase *factory();
    
    ModemArgInfoList getSettings() override;
    void writeSetting(std::string setting, std::string value) override;
    std::string readSetting(std::string setting) override;
    
    void updateDemodulatorCons(int cons_in);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    int cons;
    modemcf demodASK;
    modemcf demodASK2;
    modemcf demodASK4;
    modemcf demodASK8;
    modemcf demodASK16;
    modemcf demodASK32;
    modemcf demodASK64;
    modemcf demodASK128;
    modemcf demodASK256;
};
