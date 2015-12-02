#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemDSB : public ModemAnalog {
public:
    ModemDSB();
    ~ModemDSB();
    
    std::string getName();
    
    Modem *factory();
    
    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    ampmodem demodAM_DSB;
};