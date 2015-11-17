#include "ModemAM.h"

ModemAM::ModemAM() {
    demodAM = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 0);
}

Modem *ModemAM::factory() {
    return new ModemAM;
}

void ModemAM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitAnalog *amkit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(amkit,input);
    
    if (!bufSize) {
        input->decRefCount();
        return;
    }
    
    ampmodem_demodulate_block(demodAM, &input->data[0], bufSize, &demodOutputData[0]);
    
    buildAudioOutput(amkit,audioOut,true);
}
