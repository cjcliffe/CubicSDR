#include "ModemDPSK.h"

ModemDPSK::ModemDPSK() {
    demodDPSK2 = modem_create(LIQUID_MODEM_DPSK2);
    demodDPSK4 = modem_create(LIQUID_MODEM_DPSK4);
    demodDPSK8 = modem_create(LIQUID_MODEM_DPSK8);
    demodDPSK16 = modem_create(LIQUID_MODEM_DPSK16);
    demodDPSK32 = modem_create(LIQUID_MODEM_DPSK32);
    demodDPSK64 = modem_create(LIQUID_MODEM_DPSK64);
    demodDPSK128 = modem_create(LIQUID_MODEM_DPSK128);
    demodDPSK256 = modem_create(LIQUID_MODEM_DPSK256);
    demodulatorCons.store(2);
    currentDemodCons.store(0);
    updateDemodulatorCons(2);
}

Modem *ModemDPSK::factory() {
    return new ModemDPSK;
}

ModemDPSK::~ModemDPSK() {
    modem_destroy(demodDPSK2);
    modem_destroy(demodDPSK4);
    modem_destroy(demodDPSK8);
    modem_destroy(demodDPSK16);
    modem_destroy(demodDPSK32);
    modem_destroy(demodDPSK64);
    modem_destroy(demodDPSK128);
    modem_destroy(demodDPSK256);
}

void ModemDPSK::updateDemodulatorCons(int cons) {
    if (currentDemodCons.load() != cons) {
        currentDemodCons = cons;
        
        switch (demodulatorCons.load()) {
            case 2:
                demodDPSK = demodDPSK2;
                updateDemodulatorCons(2);
                break;
            case 4:
                demodDPSK = demodDPSK4;
                updateDemodulatorCons(4);
                break;
            case 8:
                demodDPSK = demodDPSK8;
                updateDemodulatorCons(8);
                break;
            case 16:
                demodDPSK = demodDPSK16;
                updateDemodulatorCons(16);
                break;
            case 32:
                demodDPSK = demodDPSK32;
                updateDemodulatorCons(32);
                break;
            case 64:
                demodDPSK = demodDPSK64;
                updateDemodulatorCons(64);
                break;
            case 128:
                demodDPSK = demodDPSK128;
                updateDemodulatorCons(128);
                break;
            case 256:
                demodDPSK = demodDPSK256;
                updateDemodulatorCons(256);
                break;
            default:
                demodDPSK = demodDPSK2;
                break;
        }
    }
}

void ModemDPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
   
    digitalStart(dkit, demodDPSK, input);
 
    for (int i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodDPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodDPSK, 0.005f);
    
    digitalFinish(dkit, demodDPSK);
}