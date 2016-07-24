#pragma once
#include "ModemDigital.h"

class ModemQPSK : public ModemDigital {
public:
    ModemQPSK();
    ~ModemQPSK();
    
    std::string getName();
    
    static ModemBase *factory();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodQPSK;
};
