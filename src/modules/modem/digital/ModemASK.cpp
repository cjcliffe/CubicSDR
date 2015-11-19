#include "ModemASK.h"

ModemASK::ModemASK() {
    demodASK2 = modem_create(LIQUID_MODEM_ASK2);
    demodASK4 = modem_create(LIQUID_MODEM_ASK4);
    demodASK8 = modem_create(LIQUID_MODEM_ASK8);
    demodASK16 = modem_create(LIQUID_MODEM_ASK16);
    demodASK32 = modem_create(LIQUID_MODEM_ASK32);
    demodASK64 = modem_create(LIQUID_MODEM_ASK64);
    demodASK128 = modem_create(LIQUID_MODEM_ASK128);
    demodASK256 = modem_create(LIQUID_MODEM_ASK256);
    demodASK = demodASK2;
}

Modem *ModemASK::factory() {
    return new ModemASK;
}

ModemASK::~ModemASK() {
    modem_destroy(demodASK4);
    modem_destroy(demodASK8);
    modem_destroy(demodASK16);
    modem_destroy(demodASK32);
    modem_destroy(demodASK64);
    modem_destroy(demodASK128);
    modem_destroy(demodASK256);
}

void ModemASK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {

/*
case DEMOD_TYPE_ASK:

switch (demodulatorCons.load()) {
    case 2:
        demodASK = demodASK2;
        updateDemodulatorCons(2);
        break;
    case 4:
        demodASK = demodASK4;
        updateDemodulatorCons(4);
        break;
    case 8:
        demodASK = demodASK8;
        updateDemodulatorCons(8);
        break;
    case 16:
        demodASK = demodASK16;
        updateDemodulatorCons(16);
        break;
    case 32:
        demodASK = demodASK32;
        updateDemodulatorCons(32);
        break;
    case 64:
        demodASK = demodASK64;
        updateDemodulatorCons(64);
        break;
    case 128:
        demodASK = demodASK128;
        updateDemodulatorCons(128);
        break;
    case 256:
        demodASK = demodASK256;
        updateDemodulatorCons(256);
        break;
    default:
        demodASK = demodASK2;
        break;
}

for (int i = 0; i < bufSize; i++) {
    modem_demodulate(demodASK, inp->data[i], &demodOutputDataDigital[i]);
}
updateDemodulatorLock(demodASK, 0.005f);
break;
*/
}