#include "ModemUSB.h"

ModemUSB::ModemUSB() : ModemAnalog() {
    // half band filter used for side-band elimination
    demodAM_USB = ampmodem_create(0.25, -0.25, LIQUID_AMPMODEM_USB, 1);
    ssbFilt = iirfilt_crcf_create_lowpass(6, 0.25);
    ssbShift = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_frequency(ssbShift,  (2.0 * M_PI) * 0.25);
}

Modem *ModemUSB::factory() {
    return new ModemUSB;
}

std::string ModemUSB::getName() {
    return "USB";
}

ModemUSB::~ModemUSB() {
    firfilt_crcf_destroy(ssbFilt);
    nco_crcf_destroy(ssbShift);
    ampmodem_destroy(demodAM_USB);
}

int ModemUSB::checkSampleRate(long long sampleRate, int /* audioSampleRate */) {
    if (sampleRate < MIN_BANDWIDTH) {
        return MIN_BANDWIDTH;
    }
    if (sampleRate % 2 == 0) {
        return sampleRate;
    }
    return sampleRate+1;
}

int ModemUSB::getDefaultSampleRate() {
    return 5400;
}

void ModemUSB::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitAnalog *akit = (ModemKitAnalog *)kit;
    
    initOutputBuffers(akit,input);
    
    if (!bufSize) {
        input->decRefCount();
        return;
    }
    
    liquid_float_complex x;
    for (int i = 0; i < bufSize; i++) { // Reject lower band
        nco_crcf_step(ssbShift);
        nco_crcf_mix_down(ssbShift, input->data[i], &x);
        firfilt_crcf_push(ssbFilt, x);
        firfilt_crcf_execute(ssbFilt, &x);
        ampmodem_demodulate(demodAM_USB, x, &demodOutputData[i]);
    }
    
    buildAudioOutput(akit, audioOut, true);
}

