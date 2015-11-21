#include "ModemST.h"

ModemST::ModemST() {
    demodST = modem_create(LIQUID_MODEM_V29);
}

Modem *ModemST::factory() {
    return new ModemST;
}

std::string ModemST::getName() {
    return "ST";
}

ModemST::~ModemST() {
    modem_destroy(demodST);
}

void ModemST::updateDemodulatorCons(int cons) {
    if (currentDemodCons.load() != cons) {
        currentDemodCons = cons;
    }
}

void ModemST::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitDigital *dkit = (ModemKitDigital *)kit;
    digitalStart(dkit, demodST, input);

    for (int i = 0, bufSize = input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodST, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodST, 0.005f);
    
    digitalFinish(dkit, demodST);
}