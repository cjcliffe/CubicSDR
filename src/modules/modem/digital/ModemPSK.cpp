#include "ModemPSK.h"

ModemPSK::ModemPSK() {
    demodPSK2 = modem_create(LIQUID_MODEM_PSK2);
    demodPSK4 = modem_create(LIQUID_MODEM_PSK4);
    demodPSK8 = modem_create(LIQUID_MODEM_PSK8);
    demodPSK16 = modem_create(LIQUID_MODEM_PSK16);
    demodPSK32 = modem_create(LIQUID_MODEM_PSK32);
    demodPSK64 = modem_create(LIQUID_MODEM_PSK64);
    demodPSK128 = modem_create(LIQUID_MODEM_PSK128);
    demodPSK256 = modem_create(LIQUID_MODEM_PSK256);
    demodPSK = demodPSK2;
}

Modem *ModemPSK::factory() {
    return new ModemPSK;
}

ModemPSK::~ModemPSK() {
    modem_destroy(demodPSK2);
    modem_destroy(demodPSK4);
    modem_destroy(demodPSK8);
    modem_destroy(demodPSK16);
    modem_destroy(demodPSK32);
    modem_destroy(demodPSK64);
    modem_destroy(demodPSK128);
    modem_destroy(demodPSK256);
}

void ModemPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    
    switch (demodulatorCons.load()) {
        case 2:
            demodPSK = demodPSK2;
            updateDemodulatorCons(2);
            break;
        case 4:
            demodPSK = demodPSK4;
            updateDemodulatorCons(4);
            break;
        case 8:
            demodPSK = demodPSK8;
            updateDemodulatorCons(8);
            break;
        case 16:
            demodPSK = demodPSK16;
            updateDemodulatorCons(16);
            break;
        case 32:
            demodPSK = demodPSK32;
            updateDemodulatorCons(32);
            break;
        case 64:
            demodPSK = demodPSK64;
            updateDemodulatorCons(64);
            break;
        case 128:
            demodPSK = demodPSK128;
            updateDemodulatorCons(128);
            break;
        case 256:
            demodPSK = demodPSK256;
            updateDemodulatorCons(256);
            break;
        default:
            demodPSK = demodPSK2;
            break;
    }
    
    for (int i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodPSK, 0.005f);
}