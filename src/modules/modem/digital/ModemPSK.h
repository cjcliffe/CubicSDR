#pragma once
#include "ModemDigital.h"

class ModemPSK : public ModemDigital {
public:
    ModemPSK();
    ~ModemPSK();
    
    std::string getName();
    
    static ModemBase *factory();
    
    ModemArgInfoList getSettings();
    void writeSetting(std::string setting, std::string value);
    std::string readSetting(std::string setting);
    
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
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

