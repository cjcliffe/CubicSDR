// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemFM.h"

ModemFM::ModemFM() : ModemAnalog() {
    demodFM = freqdem_create(0.5);
}

ModemFM::~ModemFM() {
    freqdem_destroy(demodFM);
}

ModemBase *ModemFM::factory() {
    return new ModemFM;
}

std::string ModemFM::getName() {
    return "FM";
}

int ModemFM::getDefaultSampleRate() {
    return 200000;
}

void ModemFM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    auto *fmkit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(fmkit, input);
    
    if (!bufSize) {
      
        return;
    }
    
    freqdem_demodulate_block(demodFM, &input->data[0], (int)bufSize, &demodOutputData[0]);

    buildAudioOutput(fmkit, audioOut, false);
}
