// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SDRDeviceInfo.h"
#include <cstdlib>
#include <algorithm>

SDRDeviceInfo::SDRDeviceInfo() : name(""), serial(""), available(false), remote(false), manual(false), soapyDevice(nullptr) {
    active.store(false);
}

SDRDeviceInfo::~SDRDeviceInfo() {
    if (soapyDevice != nullptr) {
        SoapySDR::Device::unmake(soapyDevice);
    }
}

std::string SDRDeviceInfo::getDeviceId() {
    std::string deviceId;
    
    deviceId.append(getName());
//    deviceId.append(" :: ");
//    deviceId.append(getSerial());
    
    return deviceId;
}

int SDRDeviceInfo::getIndex() const {
    return index;
}

void SDRDeviceInfo::setIndex(const int index) {
    this->index = index;
}

bool SDRDeviceInfo::isAvailable() const {
    return available;
}

void SDRDeviceInfo::setAvailable(bool available) {
    this->available = available;
}

bool SDRDeviceInfo::isActive() const {
    return active.load();
}

void SDRDeviceInfo::setActive(bool active) {
    this->active.store(active);
}

const std::string& SDRDeviceInfo::getName() const {
    return name;
}

void SDRDeviceInfo::setName(const std::string& name) {
    this->name = name;
}

const std::string& SDRDeviceInfo::getSerial() const {
    return serial;
}

void SDRDeviceInfo::setSerial(const std::string& serial) {
    this->serial = serial;
}

const std::string& SDRDeviceInfo::getTuner() const {
    return tuner;
}

void SDRDeviceInfo::setTuner(const std::string& tuner) {
    this->tuner = tuner;
}

const std::string& SDRDeviceInfo::getManufacturer() const {
    return manufacturer;
}

void SDRDeviceInfo::setManufacturer(const std::string& manufacturer) {
    this->manufacturer = manufacturer;
}

const std::string& SDRDeviceInfo::getProduct() const {
    return product;
}

void SDRDeviceInfo::setProduct(const std::string& product) {
    this->product = product;
}

const std::string& SDRDeviceInfo::getDriver() const {
    return driver;
}

void SDRDeviceInfo::setDriver(const std::string& driver) {
    this->driver = driver;
}

const std::string& SDRDeviceInfo::getHardware() const {
    return hardware;
}

void SDRDeviceInfo::setHardware(const std::string& hardware) {
    this->hardware = hardware;
}

bool SDRDeviceInfo::hasTimestamps() const {
    return timestamps;
}

void SDRDeviceInfo::setTimestamps(bool timestamps) {
    this->timestamps = timestamps;
}

bool SDRDeviceInfo::isRemote() const {
    return remote;
}

void SDRDeviceInfo::setRemote(bool remote) {
    this->remote = remote;
}

bool SDRDeviceInfo::isManual() const {
    return manual;
}

void SDRDeviceInfo::setManual(bool manual) {
    this->manual = manual;
}

void SDRDeviceInfo::setManualParams(std::string manualParams) {
    this->manual_params = manualParams;
}

std::string SDRDeviceInfo::getManualParams() {
    return manual_params;
}

void SDRDeviceInfo::setDeviceArgs(SoapySDR::Kwargs deviceArgs) {
    this->deviceArgs = deviceArgs;
}

SoapySDR::Kwargs SDRDeviceInfo::getDeviceArgs() {
    return deviceArgs;
}

void SDRDeviceInfo::setStreamArgs(SoapySDR::Kwargs streamArgs) {
    this->streamArgs = streamArgs;
}

SoapySDR::Kwargs SDRDeviceInfo::getStreamArgs() {
    return streamArgs;
}

void SDRDeviceInfo::setSoapyDevice(SoapySDR::Device *dev) {
    if (soapyDevice) {
        SoapySDR::Device::unmake(soapyDevice);
    }
    soapyDevice = dev;
}

SoapySDR::Device *SDRDeviceInfo::getSoapyDevice() {
    if (soapyDevice == nullptr) {
        soapyDevice = SoapySDR::Device::make(deviceArgs);
    }
    return soapyDevice;
}

bool SDRDeviceInfo::hasCORR(int direction, size_t channel) {
    SoapySDR::Device *dev = getSoapyDevice();
    
    std::vector<std::string> freqs = dev->listFrequencies(direction, channel);
    if (std::find(freqs.begin(), freqs.end(), "CORR") != freqs.end()) {
        return true;
    } else {
        return false;
    }
}

std::vector<long> SDRDeviceInfo::getSampleRates(int direction, size_t channel) {
    SoapySDR::Device *dev = getSoapyDevice();
    
    std::vector<long> result;
    std::vector<double> sampleRates = dev->listSampleRates(direction, channel);
    for (double si : sampleRates) {
        result.push_back((long)si);
    }
    
    return result;
}

std::vector<std::string> SDRDeviceInfo::getAntennaNames(int direction, size_t channel) {

    SoapySDR::Device *dev = getSoapyDevice();

    if (dev) {

        return  dev->listAntennas(direction, channel);
    }

    return std::vector<std::string>(); 
}

std::string SDRDeviceInfo::getAntennaName(int direction, size_t channel) {
    SoapySDR::Device *dev = getSoapyDevice();
    
    if (dev) {
        return  dev->getAntenna(direction, channel);
    }

    return std::string("");

}

long SDRDeviceInfo::getSampleRateNear(int direction, size_t channel, long sampleRate_in) {
    std::vector<long> sampleRates = getSampleRates(direction, channel);
    long returnRate = sampleRates[0];
    long sDelta = (long)sampleRate_in-sampleRates[0];
    long minDelta = std::abs(sDelta);
    for (long i : sampleRates) {
        long thisDelta = std::abs(sampleRate_in - i);
        if (thisDelta < minDelta) {
            minDelta = thisDelta;
            returnRate = i;
        }
    }
    return returnRate;
}

SDRRangeMap SDRDeviceInfo::getGains(int direction, size_t channel) {
    SoapySDR::Device *dev = getSoapyDevice();
    std::vector<std::string> gainNames = dev->listGains(direction, channel);
    std::map<std::string, SoapySDR::Range> gainMap;
    
    for (std::string gname : gainNames) {
        gainMap[gname] = dev->getGainRange(direction, channel, gname);
    }
    
    return gainMap;
}
