#pragma once
#include "ModemDigital.h"

class ModemBPSK : public ModemDigital {
public:
    ModemBPSK();
    ~ModemBPSK();
    Modem *factory();
    void updateDemodulatorCons(int cons);
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodBPSK;

};
