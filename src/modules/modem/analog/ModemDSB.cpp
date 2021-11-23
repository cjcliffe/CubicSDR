// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemDSB.h"

ModemDSB::ModemDSB() : ModemAnalogVC() {
    demodAM_DSB = ampmodem_create(0.5, LIQUID_AMPMODEM_DSB, 1);
    useSignalOutput(true);
}

ModemDSB::~ModemDSB() {
    ampmodem_destroy(demodAM_DSB);
}

ModemBase *ModemDSB::factory() {
    return new ModemDSB;
}

std::string ModemDSB::getName() {
    return "DSB";
}

int ModemDSB::getDefaultSampleRate() {
    return 5400;
}

void ModemDSB::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    auto *amkit = (ModemKitAnalog *)kit;

    initOutputBuffers(amkit, input);

    if (!bufSize) {

        return;
    }

	for (size_t i = 0; i < bufSize; i++) {
		ampmodem_demodulate(demodAM_DSB, input->data[i], &demodOutputData[i]);
	}
    applyGain(demodOutputData);
    buildAudioOutput(amkit, audioOut, false);
}
