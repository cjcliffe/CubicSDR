#include "ModemSQAM.h"

ModemSQAM::ModemSQAM() {
    demodSQAM32 = modem_create(LIQUID_MODEM_SQAM32);
    demodSQAM128 = modem_create(LIQUID_MODEM_SQAM128);
    demodSQAM = demodSQAM32;
}

Modem *ModemSQAM::factory() {
    return new ModemSQAM;
}

ModemSQAM::~ModemSQAM() {
    modem_destroy(demodSQAM32);
    modem_destroy(demodSQAM128);
}

void ModemSQAM::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    
    switch (demodulatorCons.load()) {
        case 2:
            demodSQAM = demodSQAM32;
            updateDemodulatorCons(32);
            break;
        case 4:
            demodSQAM = demodSQAM32;
            updateDemodulatorCons(32);
            break;
        case 8:
            demodSQAM = demodSQAM32;
            updateDemodulatorCons(32);
            break;
        case 16:
            demodSQAM = demodSQAM32;
            updateDemodulatorCons(32);
            break;
        case 32:
            demodSQAM = demodSQAM32;
            updateDemodulatorCons(32);
            break;
        case 64:
            demodSQAM = demodSQAM32;
            updateDemodulatorCons(32);
            break;
        case 128:
            demodSQAM = demodSQAM128;
            updateDemodulatorCons(128);
            break;
        case 256:
            demodSQAM = demodSQAM128;
            updateDemodulatorCons(128);
            break;
        default:
            demodSQAM = demodSQAM32;
            break;
    }
    
    for (int i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodSQAM, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodSQAM, 0.005f);
}