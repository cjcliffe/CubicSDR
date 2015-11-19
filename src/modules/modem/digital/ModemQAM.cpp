#include "ModemQAM.h"

ModemQAM::ModemQAM() {
    demodQAM4 = modem_create(LIQUID_MODEM_QAM4);
    demodQAM8 = modem_create(LIQUID_MODEM_QAM8);
    demodQAM16 = modem_create(LIQUID_MODEM_QAM16);
    demodQAM32 = modem_create(LIQUID_MODEM_QAM32);
    demodQAM64 = modem_create(LIQUID_MODEM_QAM64);
    demodQAM128 = modem_create(LIQUID_MODEM_QAM128);
    demodQAM256 = modem_create(LIQUID_MODEM_QAM256);
    demodQAM = demodQAM4;
}

Modem *ModemQAM::factory() {
    return new ModemQAM;
}

ModemQAM::~ModemQAM() {
    modem_destroy(demodQAM4);
    modem_destroy(demodQAM8);
    modem_destroy(demodQAM16);
    modem_destroy(demodQAM32);
    modem_destroy(demodQAM64);
    modem_destroy(demodQAM128);
    modem_destroy(demodQAM256);
}

void ModemQAM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {

/*
case DEMOD_TYPE_QAM:

switch (demodulatorCons.load()) {
    case 2:
        demodQAM = demodQAM4;
        updateDemodulatorCons(4);
        break;
    case 4:
        demodQAM = demodQAM4;
        updateDemodulatorCons(4);
        break;
    case 8:
        demodQAM = demodQAM8;
        updateDemodulatorCons(8);
        break;
    case 16:
        demodQAM = demodQAM16;
        updateDemodulatorCons(16);
        break;
    case 32:
        demodQAM = demodQAM32;
        updateDemodulatorCons(32);
        break;
    case 64:
        demodQAM = demodQAM64;
        updateDemodulatorCons(64);
        break;
    case 128:
        demodQAM = demodQAM128;
        updateDemodulatorCons(128);
        break;
    case 256:
        demodQAM = demodQAM256;
        updateDemodulatorCons(256);
        break;
    default:
        demodQAM = demodQAM4;
        break;
}

for (int i = 0; i < bufSize; i++) {
    modem_demodulate(demodQAM, inp->data[i], &demodOutputDataDigital[i]);
}
updateDemodulatorLock(demodQAM, 0.5f);
break;
*/
}
