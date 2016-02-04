#include "ModemOOK.h"

ModemOOK::ModemOOK() : ModemDigital()  {
    demodOOK = modem_create(LIQUID_MODEM_OOK);
}

ModemOOK::~ModemOOK() {
    modem_destroy(demodOOK);
}

std::string ModemOOK::getName() {
    return "OOK";
}

Modem *ModemOOK::factory() {
    return new ModemOOK;
}

int ModemOOK::checkSampleRate(long long sampleRate, int audioSampleRate) {
    if (sampleRate < 100) {
        return 100;
    }
    return sampleRate;
}

void ModemOOK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodOOK, input);
   
    for (size_t i = 0, bufSize=input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodOOK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodOOK, 0.005f);
    
    digitalFinish(dkit, demodOOK);
}
