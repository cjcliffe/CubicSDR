// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemQPSK.h"

ModemQPSK::ModemQPSK() : ModemDigital()  {
    demodQPSK = modemcf_create(LIQUID_MODEM_QPSK);
}

ModemBase *ModemQPSK::factory() {
    return new ModemQPSK;
}

ModemQPSK::~ModemQPSK() {
    modemcf_destroy(demodQPSK);
}

std::string ModemQPSK::getName() {
    return "QPSK";
}

void ModemQPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput * /* audioOut */) {
    auto *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodQPSK, input);

    for (size_t i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modemcf_demodulate(demodQPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodQPSK, 0.8f);
    
    digitalFinish(dkit, demodQPSK);
}
