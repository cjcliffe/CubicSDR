#include "Modem.h"
#include "CubicSDR.h"


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

//! Get the range minimum
double ModemRange::minimum(void) const {
    return _min;
}

//! Get the range maximum
double ModemRange::maximum(void) const {
    return _max;
}

ModemArgInfo::ModemArgInfo(void) {
    
}

Modem::Modem() {
    
}

Modem::~Modem() {
    
}

void Modem::addModemFactory(Modem *factorySingle) {
    modemFactories[factorySingle->getName()] = factorySingle;
}

ModemFactoryList Modem::getFactories() {
    return modemFactories;
}

Modem *Modem::makeModem(std::string modemName) {
    if (modemFactories.find(modemName) != modemFactories.end()) {
        return modemFactories[modemName]->factory();
    }
    
    return nullptr;
}

int Modem::getModemDefaultSampleRate(std::string modemName) {
    if (modemFactories.find(modemName) != modemFactories.end()) {
        return modemFactories[modemName]->getDefaultSampleRate();
    }
    
    return 0;
}

ModemArgInfoList Modem::getSettings() {
    ModemArgInfoList args;
    
    return args;
}

int Modem::getDefaultSampleRate() {
    return 200000;
}

void Modem::writeSetting(std::string setting, std::string value) {
    // ...
}

std::string Modem::readSetting(std::string setting) {
    return "";
}

void Modem::writeSettings(ModemSettings settings) {
    for (ModemSettings::const_iterator i = settings.begin(); i != settings.end(); i++) {
        writeSetting(i->first, i->second);
    }
}

ModemSettings Modem::readSettings() {
    ModemArgInfoList args = getSettings();
    ModemSettings rs;
    for (ModemArgInfoList::const_iterator i = args.begin(); i != args.end(); i++) {
        rs[i->key] = readSetting(i->key);
    }
    return rs;
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
