#pragma once
#include "ModemDigital.h"

class ModemASK : public ModemDigital {
public:
    ModemASK();
    ~ModemASK();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodASK;
    modem demodASK2;
    modem demodASK4;
    modem demodASK8;
    modem demodASK16;
    modem demodASK32;
    modem demodASK64;
    modem demodASK128;
    modem demodASK256;
};
