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


void ModemOOK::updateDemodulatorCons(int cons) {
    if (currentDemodCons.load() != cons) {
        currentDemodCons = cons;
    }
}

void ModemOOK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodOOK, input);
   
    for (int i = 0, bufSize=input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodOOK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodOOK, 0.005f);
    
    digitalFinish(dkit, demodOOK);
}