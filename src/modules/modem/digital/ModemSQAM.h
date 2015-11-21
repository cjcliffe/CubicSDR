#pragma once
#include "ModemDigital.h"

class ModemSQAM : public ModemDigital {
public:
    ModemSQAM();
    ~ModemSQAM();
    std::string getName();
    Modem *factory();
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodSQAM;
    modem demodSQAM32;
    modem demodSQAM128;
};
