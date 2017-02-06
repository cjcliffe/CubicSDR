// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "Modem.h"
#include "CubicSDR.h"


ModemFactoryList Modem::modemFactories;
DefaultRatesList Modem::modemDefaultRates;

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
    useSignalOutput(false);
}

Modem::~Modem() {
    
}

void Modem::addModemFactory(ModemFactoryFn factoryFunc, std::string modemName, int defaultRate) {
    modemFactories[modemName] = factoryFunc;
    modemDefaultRates[modemName] = defaultRate;
}

ModemFactoryList Modem::getFactories() {
    return modemFactories;
}

Modem *Modem::makeModem(std::string modemName) {
    if (modemFactories.find(modemName) != modemFactories.end()) {
        return (Modem *)modemFactories[modemName]();
    }
    
    return nullptr;
}

int Modem::getModemDefaultSampleRate(std::string modemName) {
    if (modemDefaultRates.find(modemName) != modemDefaultRates.end()) {
        return modemDefaultRates[modemName];
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

void Modem::writeSetting(std::string /* setting */, std::string /* value */) {
    // ...
}

std::string Modem::readSetting(std::string /* setting */) {
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


bool Modem::useSignalOutput() {
    return _useSignalOutput.load();
}

void Modem::useSignalOutput(bool useOutput) {
    _useSignalOutput.store(useOutput);
}
