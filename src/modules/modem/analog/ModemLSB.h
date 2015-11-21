#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemLSB : public ModemAnalog {
public:
    ModemLSB();
    ~ModemLSB();
    std::string getName();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    resamp2_crcf ssbFilt;
    ampmodem demodAM_LSB;
};