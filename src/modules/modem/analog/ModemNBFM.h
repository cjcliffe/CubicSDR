#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemNBFM : public ModemAnalog {
public:
    ModemNBFM();
    ~ModemNBFM();
    
    std::string getName();
    
    Modem *factory();

    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);

private:
    freqdem demodFM;
};