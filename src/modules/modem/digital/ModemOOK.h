#pragma once
#include "ModemDigital.h"

class ModemOOK : public ModemDigital {
public:
    ModemOOK();
    ~ModemOOK();
    
    std::string getName();
    
    Modem *factory();
    
    int checkSampleRate(long long sampleRate, int audioSampleRate);

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodOOK;
};
