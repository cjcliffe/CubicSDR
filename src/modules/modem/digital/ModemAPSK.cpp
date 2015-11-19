#include "ModemAPSK.h"

ModemAPSK::ModemAPSK() {
    demodAPSK4 = modem_create(LIQUID_MODEM_APSK4);
    demodAPSK8 = modem_create(LIQUID_MODEM_APSK8);
    demodAPSK16 = modem_create(LIQUID_MODEM_APSK16);
    demodAPSK32 = modem_create(LIQUID_MODEM_APSK32);
    demodAPSK64 = modem_create(LIQUID_MODEM_APSK64);
    demodAPSK128 = modem_create(LIQUID_MODEM_APSK128);
    demodAPSK256 = modem_create(LIQUID_MODEM_APSK256);
    demodAPSK = demodAPSK4;
}

Modem *ModemAPSK::factory() {
    return new ModemAPSK;
}

ModemAPSK::~ModemAPSK() {
    modem_destroy(demodAPSK4);
    modem_destroy(demodAPSK8);
    modem_destroy(demodAPSK16);
    modem_destroy(demodAPSK32);
    modem_destroy(demodAPSK64);
    modem_destroy(demodAPSK128);
    modem_destroy(demodAPSK256);
}

void ModemAPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
/*
 case DEMOD_TYPE_APSK:
    
    switch (demodulatorCons.load()) {
        case 2:
            demodAPSK = demodAPSK4;
            updateDemodulatorCons(4);
            break;
        case 4:
            demodAPSK = demodAPSK4;
            updateDemodulatorCons(4);
            break;
        case 8:
            demodAPSK = demodAPSK8;
            updateDemodulatorCons(8);
            break;
        case 16:
            demodAPSK = demodAPSK16;
            updateDemodulatorCons(16);
            break;
        case 32:
            demodAPSK = demodAPSK32;
            updateDemodulatorCons(32);
            break;
        case 64:
            demodAPSK = demodAPSK64;
            updateDemodulatorCons(64);
            break;
        case 128:
            demodAPSK = demodAPSK128;
            updateDemodulatorCons(128);
            break;
        case 256:
            demodAPSK = demodAPSK256;
            updateDemodulatorCons(256);
            break;
        default:
            demodAPSK = demodAPSK4;
            break;
    }
    
    for (int i = 0; i < bufSize; i++) {
        modem_demodulate(demodAPSK, inp->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodAPSK, 0.005f);
    break;
*/
}
