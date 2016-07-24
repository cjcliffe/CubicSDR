#pragma once
#include "ModemDigital.h"

class ModemQAM : public ModemDigital {
public:
    ModemQAM();
    ~ModemQAM();
    
    std::string getName();
    
    static ModemBase *factory();
    
    ModemArgInfoList getSettings();
    void writeSetting(std::string setting, std::string value);
    std::string readSetting(std::string setting);
    
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
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


