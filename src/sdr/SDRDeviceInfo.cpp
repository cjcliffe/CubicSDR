#include "SDRDeviceInfo.h"

SDRDeviceRange::SDRDeviceRange() {
    low = 0;
    high = 0;
}

SDRDeviceRange::SDRDeviceRange(double low, double high) {
    this->low = low;
    this->high = high;
}

double SDRDeviceRange::getLow() {
    return low;
}
void SDRDeviceRange::setLow(double low) {
    this->low = low;
}
double SDRDeviceRange::getHigh() {
    return high;
}
void SDRDeviceRange::setHigh(double high) {
    this->high = high;
}

SDRDeviceChannel::SDRDeviceChannel() {
    hardwareDC = false;
    hasCorr = false;
}

SDRDeviceChannel::~SDRDeviceChannel() {
    
}

int SDRDeviceChannel::getChannel() {
    return channel;
}

void SDRDeviceChannel::setChannel(int channel) {
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

SDRDeviceRange &SDRDeviceChannel::getGain() {
    return rangeGain;
}

SDRDeviceRange &SDRDeviceChannel::getLNAGain() {
    return rangeLNA;
}

SDRDeviceRange &SDRDeviceChannel::getFreqRange() {
    return rangeFull;
}

SDRDeviceRange &SDRDeviceChannel::getRFRange() {
    return rangeRF;
}

std::vector<long long> &SDRDeviceChannel::getSampleRates() {
    return sampleRates;
}

std::vector<long long> &SDRDeviceChannel::getFilterBandwidths() {
    return filterBandwidths;
}

const bool& SDRDeviceChannel::hasHardwareDC() const {
    return hardwareDC;
}

void SDRDeviceChannel::setHardwareDC(const bool& hardware) {
    hardwareDC = hardware;
}



const bool& SDRDeviceChannel::hasCORR() const {
    return hasCorr;
}

void SDRDeviceChannel::setCORR(const bool& hasCorr) {
	this->hasCorr = hasCorr;
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
}

void SDRDeviceInfo::setStreamArgs(SoapySDR::Kwargs streamArgs) {
    this->streamArgs = streamArgs;
}

SoapySDR::Kwargs SDRDeviceInfo::getStreamArgs() {
    return streamArgs;
}

void SDRDeviceInfo::addChannel(SDRDeviceChannel *chan) {
    channels.push_back(chan);
}

std::vector<SDRDeviceChannel *> &SDRDeviceInfo::getChannels() {
    return channels;
}

SDRDeviceChannel * SDRDeviceInfo::getRxChannel() {
    std::vector<SDRDeviceChannel *>::iterator channel_i;
    for (channel_i = channels.begin(); channel_i != channels.end(); channel_i++) {
        if ((*channel_i)->isRx()) {
            return (*channel_i);
        }
    }
    return NULL;
}

SDRDeviceChannel * SDRDeviceInfo::getTxChannel() {
    std::vector<SDRDeviceChannel *>::iterator channel_i;
    for (channel_i = channels.begin(); channel_i != channels.end(); channel_i++) {
        if ((*channel_i)->isTx()) {
            return (*channel_i);
        }
    }
    return NULL;
}

