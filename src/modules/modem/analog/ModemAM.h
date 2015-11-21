#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemAM : public ModemAnalog {
public:
    ModemAM();
    std::string getName();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    ampmodem demodAM;
};