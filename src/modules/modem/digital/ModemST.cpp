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

/*
case DEMOD_TYPE_ST:
for (int i = 0; i < bufSize; i++) {
    modem_demodulate(demodST, inp->data[i], &demodOutputDataDigital[i]);
}
updateDemodulatorLock(demodST, 0.005f);
break;

*/
}