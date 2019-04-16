// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModemUSB.h"

ModemUSB::ModemUSB() : ModemAnalog() {
    // half band filter used for side-band elimination
	ssbFilt = iirfilt_crcf_create_lowpass(6, 0.25);
	ssbShift = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_frequency(ssbShift,  (float)((2.0 * M_PI) * 0.25));
    c2rFilt = firhilbf_create(5, 90.0);
    useSignalOutput(true);
}

ModemBase *ModemUSB::factory() {
    return new ModemUSB;
}

std::string ModemUSB::getName() {
    return "USB";
}

ModemUSB::~ModemUSB() {
	iirfilt_crcf_destroy(ssbFilt);
	nco_crcf_destroy(ssbShift);
    firhilbf_destroy(c2rFilt);
}

int ModemUSB::checkSampleRate(long long sampleRate, int /* audioSampleRate */) {
    if (sampleRate < MIN_BANDWIDTH) {
        return MIN_BANDWIDTH;
    }
    if (sampleRate % 2 == 0) {
        return (int)sampleRate;
    }
    return (int)(sampleRate+1);
}

int ModemUSB::getDefaultSampleRate() {
    return 5400;
}

void ModemUSB::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitAnalog *akit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(akit,input);
    
    if (!bufSize) {
       
        return;
    }
    
    liquid_float_complex x, y;
    for (size_t i = 0; i < bufSize; i++) { // Reject lower band
        nco_crcf_step(ssbShift);
        nco_crcf_mix_down(ssbShift, input->data[i], &x);
		iirfilt_crcf_execute(ssbFilt, x, &y);
		nco_crcf_mix_up(ssbShift, y, &x);
        // TODO: use fixed firhilbf_c2r usb param to simplify instead of shifting and discarding
        float usb_discard;
        firhilbf_c2r_execute(c2rFilt, x, &demodOutputData[i], &usb_discard);
    }
    
    buildAudioOutput(akit, audioOut, true);
}

