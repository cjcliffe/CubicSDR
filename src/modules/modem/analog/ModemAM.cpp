// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemAM.h"

ModemAM::ModemAM() : ModemAnalog() {
    demodAM = ampmodem_create(0.5, LIQUID_AMPMODEM_DSB, 0);
    useSignalOutput(true);
}

ModemAM::~ModemAM() {
    ampmodem_destroy(demodAM);
}

ModemBase *ModemAM::factory() {
    return new ModemAM;
}

std::string ModemAM::getName() {
    return "AM";
}

int ModemAM::getDefaultSampleRate() {
    return 6000;
}

void ModemAM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput* audioOut) {
    ModemKitAnalog *amkit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(amkit,input);
    
    if (!bufSize) {
       
        return;
    }
    
	for (size_t i = 0; i < bufSize; i++) {
		ampmodem_demodulate(demodAM, input->data[i], &demodOutputData[i]);
	}
    
    buildAudioOutput(amkit,audioOut,true);
}
