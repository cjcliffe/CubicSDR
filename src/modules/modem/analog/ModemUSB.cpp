#include "ModemUSB.h"

ModemUSB::ModemUSB() {
    // half band filter used for side-band elimination
    ssbFilt = resamp2_crcf_create(12,-0.25f,60.0f);
    demodAM_USB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_USB, 1);
}

Modem *ModemUSB::factory() {
    return new ModemUSB;
}

ModemUSB::~ModemUSB() {
    resamp2_crcf_destroy(ssbFilt);
    ampmodem_destroy(demodAM_USB);
}

void ModemUSB::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitAnalog *akit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(akit,input);
    
    if (!bufSize) {
        input->decRefCount();
        return;
    }
    
    liquid_float_complex x, y;
    for (int i = 0; i < bufSize; i++) { // Reject lower band
        resamp2_crcf_filter_execute(ssbFilt,input->data[i],&x,&y);
        ampmodem_demodulate(demodAM_USB, y, &demodOutputData[i]);
    }
    
    buildAudioOutput(akit, audioOut, true);
}

