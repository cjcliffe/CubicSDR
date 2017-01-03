// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemNBFM.h"

ModemNBFM::ModemNBFM() : ModemAnalog() {
    demodFM = freqdem_create(0.5);
}

ModemNBFM::~ModemNBFM() {
    freqdem_destroy(demodFM);
}

ModemBase *ModemNBFM::factory() {
    return new ModemNBFM;
}

std::string ModemNBFM::getName() {
    return "NBFM";
}

int ModemNBFM::getDefaultSampleRate() {
    return 12500;
}

void ModemNBFM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitAnalog *fmkit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(fmkit, input);
    
    if (!bufSize) {
        input->decRefCount();
        return;
    }
    
    freqdem_demodulate_block(demodFM, &input->data[0], bufSize, &demodOutputData[0]);

    buildAudioOutput(fmkit, audioOut, false);
}
