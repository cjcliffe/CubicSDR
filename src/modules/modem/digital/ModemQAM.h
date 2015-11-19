#pragma once
#include "ModemDigital.h"

class ModemQAM : public ModemDigital {
public:
    ModemQAM();
    ~ModemQAM();
    Modem *factory();
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodQAM;
    modem demodQAM4;
    modem demodQAM8;
    modem demodQAM16;
    modem demodQAM32;
    modem demodQAM64;
    modem demodQAM128;
    modem demodQAM256;
};


