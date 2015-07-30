#include "SDRDeviceInfo.h"


SDRDeviceInfo::SDRDeviceInfo() : name(""), serial(""), available(false) {

}

std::string SDRDeviceInfo::getDeviceId() {
    std::string deviceId;
    
    deviceId.append(getName());
    deviceId.append(" :: ");
    deviceId.append(getSerial());
    
    return deviceId;
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

