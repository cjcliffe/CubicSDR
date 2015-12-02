#include "ModemLSB.h"

ModemLSB::ModemLSB() : ModemAnalog() {
    // half band filter used for side-band elimination
    ssbFilt = resamp2_crcf_create(12,-0.25f,60.0f);
    demodAM_LSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_LSB, 1);
}

Modem *ModemLSB::factory() {
    return new ModemLSB;
}

std::string ModemLSB::getName() {
    return "LSB";
}

ModemLSB::~ModemLSB() {
    resamp2_crcf_destroy(ssbFilt);
    ampmodem_destroy(demodAM_LSB);
}

int ModemLSB::checkSampleRate(long long sampleRate, int audioSampleRate) {
    if (sampleRate < MIN_BANDWIDTH) {
        return MIN_BANDWIDTH;
    }
    if (sampleRate % 2 == 0) {
        return sampleRate;
    }
    return sampleRate+1;
}

int ModemLSB::getDefaultSampleRate() {
    return 5400;
}

void ModemLSB::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitAnalog *akit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(akit,input);
    
    if (!bufSize) {
        input->decRefCount();
        return;
    }
    
    liquid_float_complex x, y;
    for (int i = 0; i < bufSize; i++) { // Reject upper band
        resamp2_crcf_filter_execute(ssbFilt,input->data[i],&x,&y);
        ampmodem_demodulate(demodAM_LSB, x, &demodOutputData[i]);
    }
    
    buildAudioOutput(akit, audioOut, true);
}
