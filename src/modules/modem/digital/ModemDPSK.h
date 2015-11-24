#pragma once
#include "ModemDigital.h"

class ModemDPSK : public ModemDigital {
public:
    ModemDPSK();
    ~ModemDPSK();

    std::string getName();
    
    Modem *factory();
    
    ModemArgInfoList getSettings();
    void writeSetting(std::string setting, std::string value);
    std::string readSetting(std::string setting);
    
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
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

