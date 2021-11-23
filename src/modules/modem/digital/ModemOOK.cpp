// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemOOK.h"

ModemOOK::ModemOOK() : ModemDigital()  {
    demodOOK = modemcf_create(LIQUID_MODEM_OOK);
}

ModemOOK::~ModemOOK() {
    modemcf_destroy(demodOOK);
}

std::string ModemOOK::getName() {
    return "OOK";
}

ModemBase *ModemOOK::factory() {
    return new ModemOOK;
}

int ModemOOK::checkSampleRate(long long sampleRate, int /* audioSampleRate */) {
    if (sampleRate < 100) {
        return 100;
    }
    return (int)sampleRate;
}

void ModemOOK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput * /* audioOut */) {
    auto *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodOOK, input);
   
    for (size_t i = 0, bufSize=input->data.size(); i < bufSize; i++) {
        modemcf_demodulate(demodOOK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodOOK, 0.005f);
    
    digitalFinish(dkit, demodOOK);
}
