#pragma once
#include "ModemDigital.h"

class ModemASK : public ModemDigital {
public:
    ModemASK();
    ~ModemASK();
    
    std::string getName();
    
    Modem *factory();
    
    ModemArgInfoList getSettings();
    void writeSetting(std::string setting, std::string value);
    std::string readSetting(std::string setting);
    
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    int cons;
    modem demodASK;
    modem demodASK2;
    modem demodASK4;
    modem demodASK8;
    modem demodASK16;
    modem demodASK32;
    modem demodASK64;
    modem demodASK128;
    modem demodASK256;
};
