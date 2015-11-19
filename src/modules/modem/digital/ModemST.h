#pragma once
#include "ModemDigital.h"

class ModemST : public ModemDigital {
public:
    ModemST();
    ~ModemST();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodST;
};

