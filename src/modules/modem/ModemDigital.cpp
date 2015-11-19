#include "ModemDigital.h"

ModemDigital::ModemDigital() {
    demodulatorCons.store(2);
    currentDemodCons = 0;
    currentDemodLock = false;
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
    demod_lock_in ? currentDemodLock = true : currentDemodLock = false;
}

int ModemDigital::getDemodulatorLock() {
    return currentDemodLock;
}

void ModemDigital::setDemodulatorCons(int demod_cons_in) {
    demodulatorCons.store(demod_cons_in);
}

int ModemDigital::getDemodulatorCons() {
    return currentDemodCons;
}

void ModemDigital::updateDemodulatorLock(modem demod, float sensitivity) {
    modem_get_demodulator_evm(demod) <= sensitivity ? setDemodulatorLock(true) : setDemodulatorLock(false);
}

void ModemDigital::updateDemodulatorCons(int Cons) {
    if (currentDemodCons != Cons) {
        currentDemodCons = Cons;
    }
}


// Demodulate
/*
 // Reset demodulator Constellations & Lock
 //        updateDemodulatorCons(0);

{
    switch (demodulatorType.load()) {
            // advanced demodulators
              
           }
}

 
}
 
 //		demodOutputDataDigital.empty();

 
 */
