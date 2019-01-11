// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemST.h"

ModemST::ModemST() : ModemDigital()  {
    demodST = modem_create(LIQUID_MODEM_V29);
}

ModemBase *ModemST::factory() {
    return new ModemST;
}

std::string ModemST::getName() {
    return "ST";
}

ModemST::~ModemST() {
    modem_destroy(demodST);
}

void ModemST::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput * /* audioOut */) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodST, input);

    for (size_t i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodST, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodST, 0.005f);
    
    digitalFinish(dkit, demodST);
}
