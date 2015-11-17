#include "Modem.h"

ModemFactoryList Modem::modemFactories;

void Modem::addModemFactory(std::string modemName, ModemFactoryFunc *factoryFunc) {
    modemFactories[modemName] = factoryFunc;
}

ModemFactoryList Modem::getFactories() {
    return modemFactories;
}

Modem *Modem::factory() {
    return nullptr;
}

ModemKit *Modem::buildKit(long long sampleRate, int audioSampleRate) {
    return nullptr;
}

void Modem::disposeKit(ModemKit *kit) {
    return;
}

void Modem::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    return;
}
