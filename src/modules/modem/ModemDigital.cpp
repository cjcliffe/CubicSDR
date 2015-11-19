#include "ModemDigital.h"

ModemDigital::ModemDigital() {
    demodulatorCons.store(2);
    // Reset demodulator Constellations & Lock
    updateDemodulatorCons(0);
    currentDemodLock.store(false);
}

ModemKit *ModemDigital::buildKit(long long sampleRate, int audioSampleRate) {
    ModemKitDigital *dkit = new ModemKitDigital;
    
    dkit->sampleRate = sampleRate;
    dkit->audioSampleRate = audioSampleRate;
    
    return dkit;
}

void ModemDigital::disposeKit(ModemKit *kit) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    
    delete dkit;
}


void ModemDigital::setDemodulatorLock(bool demod_lock_in) {
    currentDemodLock.store(demod_lock_in);
}

int ModemDigital::getDemodulatorLock() {
    return currentDemodLock.load();
}

void ModemDigital::setDemodulatorCons(int demod_cons_in) {
    demodulatorCons.store(demod_cons_in);
}

int ModemDigital::getDemodulatorCons() {
    return currentDemodCons.load();
}

void ModemDigital::updateDemodulatorLock(modem mod, float sensitivity) {
    modem_get_demodulator_evm(mod) <= sensitivity ? setDemodulatorLock(true) : setDemodulatorLock(false);
}

void ModemDigital::updateDemodulatorCons(int cons) {
    if (currentDemodCons.load() != cons) {
        currentDemodCons = cons;
    }
}

void ModemDigital::digitalStart(ModemKitDigital *kit, modem mod, ModemIQData *input) {
    int bufSize = input->data.size();
    
    if (demodOutputDataDigital.size() != bufSize) {
        if (demodOutputDataDigital.capacity() < bufSize) {
            demodOutputDataDigital.reserve(bufSize);
        }
        demodOutputDataDigital.resize(bufSize);
    }
    
    if (demodulatorCons.load() != currentDemodCons.load()) {
        updateDemodulatorCons(demodulatorCons.load());
        currentDemodLock.store(false);
    }
}

void ModemDigital::digitalFinish(ModemKitDigital *kit, modem mod) {
    demodOutputDataDigital.empty();
}
 
