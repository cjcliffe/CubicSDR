// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SDRDeviceInfo.h"
#include "CubicSDRDefs.h"
#include <cstdlib>
#include <algorithm>

SDRDeviceInfo::SDRDeviceInfo() : available(false), remote(false), manual(false), soapyDevice(nullptr) {
    active.store(false);
}

SDRDeviceInfo::~SDRDeviceInfo() {
    if (soapyDevice != nullptr) {
        SoapySDR::Device::unmake(soapyDevice);
    }
}

std::string SDRDeviceInfo::getDeviceId() const {
    std::string deviceId;
    
    deviceId.append(getName());
//    deviceId.append(" :: ");
//    deviceId.append(getSerial());
    
    return deviceId;
}

int SDRDeviceInfo::getIndex() const {
    return index;
}

void SDRDeviceInfo::setIndex(const int index_in) {
    index = index_in;
}

bool SDRDeviceInfo::isAvailable() const {
    return available;
}

void SDRDeviceInfo::setAvailable(bool available_in) {
    available = available_in;
}

bool SDRDeviceInfo::isActive() const {
    return active.load();
}

void SDRDeviceInfo::setActive(bool active_in) {
    active.store(active_in);
}

const std::string& SDRDeviceInfo::getName() const {
    return name;
}

void SDRDeviceInfo::setName(const std::string& name_in) {
    name = name_in;
}

const std::string& SDRDeviceInfo::getSerial() const {
    return serial;
}

void SDRDeviceInfo::setSerial(const std::string& serial_in) {
    serial = serial_in;
}

const std::string& SDRDeviceInfo::getTuner() const {
    return tuner;
}

void SDRDeviceInfo::setTuner(const std::string& tuner_in) {
    tuner = tuner_in;
}

const std::string& SDRDeviceInfo::getManufacturer() const {
    return manufacturer;
}

void SDRDeviceInfo::setManufacturer(const std::string& manufacturer_in) {
    manufacturer = manufacturer_in;
}

const std::string& SDRDeviceInfo::getProduct() const {
    return product;
}

void SDRDeviceInfo::setProduct(const std::string& product_in) {
    product = product_in;
}

const std::string& SDRDeviceInfo::getDriver() const {
    return driver;
}

void SDRDeviceInfo::setDriver(const std::string& driver_in) {
    driver = driver_in;
}

const std::string& SDRDeviceInfo::getHardware() const {
    return hardware;
}

void SDRDeviceInfo::setHardware(const std::string& hardware_in) {
    hardware = hardware_in;
}

bool SDRDeviceInfo::hasTimestamps() const {
    return timestamps;
}

void SDRDeviceInfo::setTimestamps(bool timestamps_in) {
    timestamps = timestamps_in;
}

bool SDRDeviceInfo::isRemote() const {
    return remote;
}

void SDRDeviceInfo::setRemote(bool remote_in) {
    remote = remote_in;
}

bool SDRDeviceInfo::isManual() const {
    return manual;
}

void SDRDeviceInfo::setManual(bool manual_in) {
    manual = manual_in;
}

void SDRDeviceInfo::setManualParams(std::string manualParams_in) {
    manual_params = manualParams_in;
}

std::string SDRDeviceInfo::getManualParams() {
    return manual_params;
}

void SDRDeviceInfo::setDeviceArgs(SoapySDR::Kwargs deviceArgs_in) {
    deviceArgs = deviceArgs_in;
}

SoapySDR::Kwargs SDRDeviceInfo::getDeviceArgs() {
    return deviceArgs;
}

void SDRDeviceInfo::setStreamArgs(SoapySDR::Kwargs streamArgs_in) {
    streamArgs = streamArgs_in;
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

/**
 * @deprecated
 * @param direction
 * @param channel
 * @return
 */
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

	size_t nbMaxDifferentRates = DEVICE_SAMPLE_RATES_MAX_NB;
    
    std::vector<long> result;

	//the original list returned from the driver:
    std::vector<double> sampleRates = dev->listSampleRates(direction, channel);

	//be paranoid, sort by increasing rates...
	std::sort(sampleRates.begin(), sampleRates.end(), [](double a, double b) -> bool { return a < b; });

	//if sampleRates.size() > nbMaxDifferentRates, decimate this number to only return nbMaxDifferentRates sample 
	//rates values.
	size_t sampleRateSelectionStep = 1;

	if (sampleRates.size() / nbMaxDifferentRates >= 2) {

	   sampleRateSelectionStep = sampleRates.size() / nbMaxDifferentRates;
	}
	
	for (size_t i = 0; sampleRateSelectionStep * i < sampleRates.size(); i++) {

		//convert to longs...
		result.push_back((long)sampleRates[sampleRateSelectionStep * i]);
	}

	//always include the biggest value:
	if ((long)sampleRates.back() > result.back()) {

		result.push_back((long)sampleRates.back());
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
    
    for (const std::string& gname : gainNames) {

        gainMap[gname] = dev->getGainRange(direction, channel, gname);
    }
    
    return gainMap;
}

//read the current gain of name gainName (must exit in getGains(), else return 0)
//in the device.
double  SDRDeviceInfo::getCurrentGain(int direction, size_t channel, const std::string& gainName) {

	SoapySDR::Device *dev = getSoapyDevice();

	if (dev) {

		std::vector<std::string> gainNames = dev->listGains(direction, channel);

		auto itFoundName = std::find(gainNames.begin(), gainNames.end(), gainName);

		if (itFoundName != gainNames.end()) {

			return  dev->getGain(direction, channel, gainName);
		}
	}

	return 0.0;
}
