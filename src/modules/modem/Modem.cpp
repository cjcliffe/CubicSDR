#include "Modem.h"

ModemFactoryList Modem::modemFactories;

void Modem::addModemFactory(Modem *factorySingle) {
    modemFactories[factorySingle->getName()] = factorySingle;
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

ModemArgInfoList Modem::getSettings() {
    ModemArgInfoList args;
    
    return args;
}

void Modem::writeSetting(std::string setting, std::string value) {
    // ...
}

std::string Modem::readSetting(std::string setting) {
    return "";
}

Modem::Modem() {
    
}

Modem::~Modem() {
    
}
