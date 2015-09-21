#include "SDRDeviceInfo.h"


int SDRDeviceChannel::getChannel() const {
    return channel;
}

void SDRDeviceChannel::setChannel(const int channel) {
    this->channel = channel;
}

bool SDRDeviceChannel::isFullDuplex() {
    return fullDuplex;
}

void SDRDeviceChannel::setFullDuplex(bool fullDuplex) {
    this->fullDuplex = fullDuplex;
}

bool SDRDeviceChannel::isTx() {
    return tx;
}

void SDRDeviceChannel::setTx(bool tx) {
    this->tx = tx;
}

bool SDRDeviceChannel::isRx() {
    return rx;
}

void SDRDeviceChannel::setRx(bool rx) {
    this->rx = rx;
}

const SDRDeviceRange &SDRDeviceChannel::getGain() const {
    return rangeGain;
}

const SDRDeviceRange &SDRDeviceChannel::getLNAGain() const {
    return rangeLNA;
}

const SDRDeviceRange &SDRDeviceChannel::getFreqRange() const {
    return rangeFull;
}

const SDRDeviceRange &SDRDeviceChannel::getRFRange() const {
    return rangeRF;
}

const std::vector<long long> &SDRDeviceChannel::getSampleRates() const {
    return sampleRates;
}

const std::vector<long long> &SDRDeviceChannel::getFilterBandwidths() const {
    return filterBandwidths;
}



SDRDeviceInfo::SDRDeviceInfo() : name(""), serial(""), available(false) {

}

std::string SDRDeviceInfo::getDeviceId() {
    std::string deviceId;
    
    deviceId.append(getName());
//    deviceId.append(" :: ");
//    deviceId.append(getSerial());
    
    return deviceId;
}

const int SDRDeviceInfo::getIndex() const {
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

void SDRDeviceInfo::setDeviceArgs(SoapySDR::Kwargs deviceArgs) {
    this->deviceArgs = deviceArgs;
}

SoapySDR::Kwargs SDRDeviceInfo::getDeviceArgs() {
    return deviceArgs;
//    return "driver=" + driver + "," + getDriver() + "=" + std::to_string(getIndex());
}
