#include "ModemFM.h"

ModemFM::ModemFM() {
    demodFM = freqdem_create(0.5);
}

Modem *ModemFM::factory() {
    return new ModemFM;
}

void ModemFM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitAnalog *fmkit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(fmkit, input);
    
    if (!bufSize) {
        input->decRefCount();
        return;
    }
    
    freqdem_demodulate_block(demodFM, &input->data[0], bufSize, &demodOutputData[0]);

    buildAudioOutput(fmkit, audioOut, false);
}
