// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>
#include <vector>
#include <atomic>

#include <SoapySDR/Types.hpp>
#include <SoapySDR/Device.hpp>

typedef struct SDRManualDef {
    std::string factory;
    std::string params;
} SDRManualDef;

typedef std::map<std::string, SoapySDR::Range> SDRRangeMap;

class SDRDeviceInfo {
public:
    SDRDeviceInfo();
    ~SDRDeviceInfo();
    
    std::string getDeviceId() const;
    
    int getIndex() const;
    void setIndex(int index_in);
    
    bool isAvailable() const;
    void setAvailable(bool available_in);

    bool isActive() const;
    void setActive(bool active_in);

    const std::string& getName() const;
    void setName(const std::string& name_in);
    
    const std::string& getSerial() const;
    void setSerial(const std::string& serial_in);
    
    const std::string& getTuner() const;
    void setTuner(const std::string& tuner_in);
    
    const std::string& getManufacturer() const;
    void setManufacturer(const std::string& manufacturer_in);
    
    const std::string& getProduct() const;
    void setProduct(const std::string& product_in);

    const std::string& getDriver() const;
    void setDriver(const std::string& driver_in);
    
    const std::string& getHardware() const;
    void setHardware(const std::string& hardware_in);
    
    bool hasTimestamps() const;
    void setTimestamps(bool timestamps_in);

    bool isRemote() const;
    void setRemote(bool remote_in);

    bool isManual() const;
    void setManual(bool manual_in);
    
    void setManualParams(std::string manualParams_in);
    std::string getManualParams();
    
    void setDeviceArgs(SoapySDR::Kwargs deviceArgs_in);
    SoapySDR::Kwargs getDeviceArgs();

    void setStreamArgs(SoapySDR::Kwargs streamArgs_in);
    SoapySDR::Kwargs getStreamArgs();

//    void setSettingsInfo(SoapySDR::ArgInfoList settingsArgs);
//    SoapySDR::ArgInfoList getSettingsArgInfo();

//    std::vector<std::string> getSettingNames();
    
    void setSoapyDevice(SoapySDR::Device *dev);
    SoapySDR::Device *getSoapyDevice();
    
    bool hasCORR(int direction, size_t channel);
    
    std::vector<long> getSampleRates(int direction, size_t channel);

    std::vector<std::string> getAntennaNames(int direction, size_t channel);

    std::string getAntennaName(int direction, size_t channel);
    
    long getSampleRateNear(int direction, size_t channel, long sampleRate_in);

    SDRRangeMap getGains(int direction, size_t channel);

	//read the current gain of name gainName (must exist in getGains(), else return 0)
	//in the device.
	double getCurrentGain(int direction, size_t channel, const std::string& gainName);

private:
    int index = 0;
    std::string name, serial, product, manufacturer, tuner;
    std::string driver, hardware, manual_params;
    bool timestamps{}, available, remote, manual;
    std::atomic_bool active{};
    
    SoapySDR::Kwargs deviceArgs, streamArgs;
    SoapySDR::Device *soapyDevice;
};
