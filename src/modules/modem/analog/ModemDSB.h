#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemDSB : public ModemAnalog {
public:
    ModemDSB();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    ampmodem demodAM_DSB;
};