#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemAM : public ModemAnalog {
public:
    ModemAM();
    ~ModemAM();
    
    std::string getName();
    
    Modem *factory();
    
    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    ampmodem demodAM;
};