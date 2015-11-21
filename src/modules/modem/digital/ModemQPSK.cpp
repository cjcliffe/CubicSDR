#include "ModemQPSK.h"

ModemQPSK::ModemQPSK() {
    demodQPSK = modem_create(LIQUID_MODEM_QPSK);
}

Modem *ModemQPSK::factory() {
    return new ModemQPSK;
}

ModemQPSK::~ModemQPSK() {
    modem_destroy(demodQPSK);
}

std::string ModemQPSK::getName() {
    return "QPSK";
}

void ModemQPSK::updateDemodulatorCons(int cons) {
    if (currentDemodCons.load() != cons) {
        currentDemodCons = cons;
    }
}

void ModemQPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodQPSK, input);

    for (int i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodQPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodQPSK, 0.8f);
    
    digitalFinish(dkit, demodQPSK);
}