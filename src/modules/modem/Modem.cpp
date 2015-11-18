#include "Modem.h"

ModemFactoryList Modem::modemFactories;

void Modem::addModemFactory(std::string modemName, Modem *factorySingle) {
    modemFactories[modemName] = factorySingle;
}

ModemFactoryList Modem::getFactories() {
    return modemFactories;
}

Modem *Modem::makeModem(std::string modemType) {
    if (modemFactories.find(modemType) != modemFactories.end()) {
        return modemFactories[modemType]->factory();
    }
    
    return nullptr;
}

Modem::Modem() {
    
}

Modem::~Modem() {
    
}
