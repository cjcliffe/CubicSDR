// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemST.h"

ModemST::ModemST() : ModemDigital()  {
    demodST = modemcf_create(LIQUID_MODEM_V29);
}

ModemBase *ModemST::factory() {
    return new ModemST;
}

std::string ModemST::getName() {
    return "ST";
}

ModemST::~ModemST() {
    modemcf_destroy(demodST);
}

void ModemST::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput * /* audioOut */) {
    auto *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodST, input);

    for (size_t i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modemcf_demodulate(demodST, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodST, 0.005f);
    
    digitalFinish(dkit, demodST);
}
