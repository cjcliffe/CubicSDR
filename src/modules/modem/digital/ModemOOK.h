#pragma once
#include "ModemDigital.h"

class ModemOOK : public ModemDigital {
public:
    ModemOOK();
    ~ModemOOK();
    std::string getName();
    Modem *factory();
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodOOK;

};
