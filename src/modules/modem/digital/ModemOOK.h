#pragma once
#include "ModemDigital.h"

class ModemOOK : public ModemDigital {
public:
    ModemOOK();
    ~ModemOOK();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodOOK;

};
