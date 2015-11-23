#include "ModemFSK.h"

ModemFSK::ModemFSK() {
    demodulatorCons.store(2);
    currentDemodCons.store(0);
    updateDemodulatorCons(2);
}

Modem *ModemFSK::factory() {
    return new ModemFSK;
}

ModemKit *ModemFSK::buildKit(long long sampleRate, int audioSampleRate) {
    ModemKitFSK *dkit = new ModemKitFSK;
    dkit->m           = 1;
    dkit->k           = sampleRate / 9600;
    dkit->bandwidth   = 7000.0 / sampleRate;

    dkit->demodFSK = fskdem_create(dkit->m, dkit->k, dkit->bandwidth);

    dkit->sampleRate = sampleRate;
    dkit->audioSampleRate = audioSampleRate;
 
    return dkit;
}

void ModemFSK::disposeKit(ModemKit *kit) {
    ModemKitFSK *dkit = (ModemKitFSK *)kit;
    
    delete dkit;
}

std::string ModemFSK::getName() {
    return "FSK";
}

ModemFSK::~ModemFSK() {

}

void ModemFSK::updateDemodulatorCons(int cons) {
   
}

void ModemFSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitFSK *dkit = (ModemKitFSK *)kit;
    
    digitalStart(dkit, nullptr, input);

    dkit->inputBuffer.insert(dkit->inputBuffer.end(),input->data.begin(),input->data.end());

    while (dkit->inputBuffer.size() >= dkit->k) {
        unsigned int sym_out;

        sym_out = fskdem_demodulate(dkit->demodFSK, &dkit->inputBuffer[0]);
//        std::cout << "ferror: " << fskdem_get_frequency_error(dkit->demodFSK) << std::endl;
        printf("%01X", sym_out);
        dkit->inputBuffer.erase(dkit->inputBuffer.begin(),dkit->inputBuffer.begin()+dkit->k);
    }
    
    digitalFinish(dkit, nullptr);
}