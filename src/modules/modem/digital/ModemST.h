#pragma once
#include "ModemDigital.h"

class ModemST : public ModemDigital {
public:
    ModemST();
    ~ModemST();
    std::string getName();
    Modem *factory();
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodST;
};

