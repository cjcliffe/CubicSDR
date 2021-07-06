// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once
#include "ModemDigital.h"

class ModemQAM : public ModemDigital {
public:
    ModemQAM();
    ~ModemQAM() override;
    
    std::string getName() override;
    
    static ModemBase *factory();
    
    ModemArgInfoList getSettings() override;
    void writeSetting(std::string setting, std::string value) override;
    std::string readSetting(std::string setting) override;
    
    void updateDemodulatorCons(int cons_in);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) override;
    
private:
    int cons;
    modem demodQAM;
    modem demodQAM4;
    modem demodQAM8;
    modem demodQAM16;
    modem demodQAM32;
    modem demodQAM64;
    modem demodQAM128;
    modem demodQAM256;
};


