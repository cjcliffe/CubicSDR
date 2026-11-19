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

SquelchScheme ModemNBFM::getSquelchScheme() const {
    return SquelchScheme::Noise;
}

float ModemNBFM::getSquelchMetric() const {
    return squelchMetricDb;
}

void ModemNBFM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    auto *fmkit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(fmkit, input);
    
    if (!bufSize) {
       
        return;
    }
    
    freqdem_demodulate_block(demodFM, &input->data[0], (unsigned int)bufSize, &demodOutputData[0]);
    squelchMetricDb = noiseMeter.process(&demodOutputData[0], bufSize, (float)input->sampleRate);

    buildAudioOutput(fmkit, audioOut, false);
}
