#include "ModemLSB.h"

ModemLSB::ModemLSB() : ModemAnalog() {
    // half band filter used for side-band elimination
    //    demodAM_LSB = ampmodem_create(0.25, 0.25, LIQUID_AMPMODEM_LSB, 1);
    ssbFilt = iirfilt_crcf_create_lowpass(6, 0.25);
    ssbShift = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_frequency(ssbShift,  (2.0 * M_PI) * 0.25);
    c2rFilt = firhilbf_create(5, 90.0);
}

Modem *ModemLSB::factory() {
    return new ModemLSB;
}

std::string ModemLSB::getName() {
    return "LSB";
}

ModemLSB::~ModemLSB() {
    iirfilt_crcf_destroy(ssbFilt);
    nco_crcf_destroy(ssbShift);
    firhilbf_destroy(c2rFilt);
    //    ampmodem_destroy(demodAM_LSB);
}

int ModemLSB::checkSampleRate(long long sampleRate, int /* audioSampleRate */) {
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
    for (size_t i = 0; i < bufSize; i++) { // Reject upper band
        nco_crcf_step(ssbShift);
        nco_crcf_mix_up(ssbShift, input->data[i], &x);
        iirfilt_crcf_execute(ssbFilt, x, &y);
        nco_crcf_mix_down(ssbShift, y, &x);
        // Liquid-DSP AMPModem SSB drifts with strong signals near baseband (like a carrier?)
        // ampmodem_demodulate(demodAM_LSB, y, &demodOutputData[i]);
        firhilbf_c2r_execute(c2rFilt, x, &demodOutputData[i]);
    }
    
    buildAudioOutput(akit, audioOut, true);
}
