#include "ModemST.h"

ModemST::ModemST() {
    demodST = modem_create(LIQUID_MODEM_V29);
}

Modem *ModemST::factory() {
    return new ModemST;
}

ModemST::~ModemST() {
    modem_destroy(demodST);
}

void ModemST::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    
    for (int i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodST, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodST, 0.005f);
}