// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemAM.h"

ModemAM::ModemAM() : ModemAnalogVC()
{
    // Create a DC blocker using 25 samples wide window
    // and 30dB reduction of the DC level.
    mDCBlock = firfilt_rrrf_create_dc_blocker (25,30.0f);
    useSignalOutput(true);
}

ModemAM::~ModemAM() {
    firfilt_rrrf_destroy(mDCBlock);
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

  // Implement an AM demodulator. Compute signal
  // amplitude followed by a DC blocker to remove
  // the DC offset.
	for (size_t i = 0; i < bufSize; i++) {
    float I = input->data[i].real;
    float Q = input->data[i].imag;
    firfilt_rrrf_push (mDCBlock,sqrt(I*I+Q*Q));
    firfilt_rrrf_execute (mDCBlock,&demodOutputData[i]);
	}

  applyGain (demodOutputData);

  buildAudioOutput(amkit,audioOut,false);
}
