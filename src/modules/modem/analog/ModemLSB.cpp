#include "ModemLSB.h"

ModemLSB::ModemLSB() {
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
