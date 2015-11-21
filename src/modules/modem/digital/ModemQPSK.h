#pragma once
#include "ModemDigital.h"

class ModemQPSK : public ModemDigital {
public:
    ModemQPSK();
    ~ModemQPSK();
    std::string getName();
    Modem *factory();
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodQPSK;
};
