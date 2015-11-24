#pragma once
#include "ModemDigital.h"

class ModemSQAM : public ModemDigital {
public:
    ModemSQAM();
    ~ModemSQAM();
    
    std::string getName();
    
    Modem *factory();
    
    ModemArgInfoList getSettings();
    void writeSetting(std::string setting, std::string value);
    std::string readSetting(std::string setting);

    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    int cons;
    modem demodSQAM;
    modem demodSQAM32;
    modem demodSQAM128;
};
