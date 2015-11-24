#include "Modem.h"

ModemFactoryList Modem::modemFactories;

//! Create an empty range (0.0, 0.0)
ModemRange::ModemRange(void) {
    _min = 0;
    _max = 0;
}

//! Create a min/max range
ModemRange::ModemRange(const double minimum, const double maximum) {
    _min = minimum;
    _max = maximum;
}

ModemArgInfo::ModemArgInfo(void) {
    
}

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

bool Modem::shouldRebuildKit() {
    return refreshKit.load();
}

void Modem::rebuildKit() {
    refreshKit.store(true);
}

void Modem::clearRebuildKit() {
    refreshKit.store(false);
}
