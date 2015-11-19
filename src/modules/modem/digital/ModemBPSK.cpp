#include "ModemBPSK.h"

ModemBPSK::ModemBPSK() {
    demodBPSK = modem_create(LIQUID_MODEM_BPSK);
}

Modem *ModemBPSK::factory() {
    return new ModemBPSK;
}

ModemBPSK::~ModemBPSK() {
    modem_destroy(demodBPSK);
}

void ModemBPSK::updateDemodulatorCons(int cons) {
    if (currentDemodCons.load() != cons) {
        currentDemodCons = cons;
    }
}

void ModemBPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodBPSK, input);

    for (int i = 0, bufSize=input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodBPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodBPSK, 0.005f);
    
    digitalFinish(dkit, demodBPSK);
}