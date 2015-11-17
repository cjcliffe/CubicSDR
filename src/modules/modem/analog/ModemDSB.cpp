#include "ModemDSB.h"

ModemDSB::ModemDSB() {
    demodAM_DSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 1);
}

Modem *ModemDSB::factory() {
    return new ModemDSB;
}

void ModemDSB::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitAnalog *amkit = (ModemKitAnalog *)kit;

    initOutputBuffers(amkit, input);
    
    if (!bufSize) {
        input->decRefCount();
        return;
    }
    
    ampmodem_demodulate_block(demodAM_DSB, &input->data[0], bufSize, &demodOutputData[0]);
    
    buildAudioOutput(amkit, audioOut, true);
}
