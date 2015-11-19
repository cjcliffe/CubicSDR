#include "ModemOOK.h"

ModemOOK::ModemOOK() {
    demodOOK = modem_create(LIQUID_MODEM_OOK);
}

Modem *ModemOOK::factory() {
    return new ModemOOK;
}

ModemOOK::~ModemOOK() {
    modem_destroy(demodOOK);
}

void ModemOOK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    
    for (int i = 0, bufSize=input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodOOK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodOOK, 0.005f);
}