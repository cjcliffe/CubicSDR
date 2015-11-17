#pragma once
#include "Modem.h"
#include "ModemAnalog.h"

class ModemFM : public ModemAnalog {
public:
    ModemFM();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);

private:
 
    freqdem demodFM;
};