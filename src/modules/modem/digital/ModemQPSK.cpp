#include "ModemQPSK.h"

ModemQPSK::ModemQPSK() : ModemDigital()  {
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

void ModemQPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodQPSK, input);

    for (size_t i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodQPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodQPSK, 0.8f);
    
    digitalFinish(dkit, demodQPSK);
}
