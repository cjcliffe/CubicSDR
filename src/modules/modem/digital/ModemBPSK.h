#pragma once
#include "ModemDigital.h"

class ModemBPSK : public ModemDigital {
public:
    ModemBPSK();
    ~ModemBPSK();
    
    std::string getName();
    
    static ModemBase *factory();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodBPSK;
};
