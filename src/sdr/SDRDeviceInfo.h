// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>
#include <vector>
#include <atomic>

#include <SoapySDR/Types.hpp>
#include <SoapySDR/Device.hpp>

typedef struct _SDRManualDef {
    std::string factory;
    std::string params;
} SDRManualDef;

typedef std::map<std::string, SoapySDR::Range> SDRRangeMap;

class SDRDeviceInfo {
public:
    SDRDeviceInfo();
    ~SDRDeviceInfo();
    
    std::string getDeviceId();
    
    int getIndex() const;
    void setIndex(const int index);
    
    bool isAvailable() const;
    void setAvailable(bool available);

    bool isActive() const;
    void setActive(bool active);

    const std::string& getName() const;
    void setName(const std::string& name);
    
    const std::string& getSerial() const;
    void setSerial(const std::string& serial);
    
    const std::string& getTuner() const;
    void setTuner(const std::string& tuner);
    
    const std::string& getManufacturer() const;
    void setManufacturer(const std::string& manufacturer);
    
    const std::string& getProduct() const;
    void setProduct(const std::string& product);

    const std::string& getDriver() const;
    void setDriver(const std::string& driver);
    
    const std::string& getHardware() const;
    void setHardware(const std::string& hardware);
    
    bool hasTimestamps() const;
    void setTimestamps(bool timestamps);

    bool isRemote() const;
    void setRemote(bool remote);

    bool isManual() const;
    void setManual(bool manual);
    
    void setManualParams(std::string manualParams);
    std::string getManualParams();
    
    void setDeviceArgs(SoapySDR::Kwargs deviceArgs);
    SoapySDR::Kwargs getDeviceArgs();

    void setStreamArgs(SoapySDR::Kwargs deviceArgs);
    SoapySDR::Kwargs getStreamArgs();

//    void setSettingsInfo(SoapySDR::ArgInfoList settingsArgs);
//    SoapySDR::ArgInfoList getSettingsArgInfo();

//    std::vector<std::string> getSettingNames();
    
    void setSoapyDevice(SoapySDR::Device *dev);
    SoapySDR::Device *getSoapyDevice();
    
    bool hasCORR(int direction, size_t channel);
    
    std::vector<long> getSampleRates(int direction, size_t channel);
    
    long getSampleRateNear(int direction, size_t channel, long sampleRate_in);

    SDRRangeMap getGains(int direction, size_t channel);

private:
    int index;
    std::string name, serial, product, manufacturer, tuner;
    std::string driver, hardware, manual_params;
    bool timestamps, available, remote, manual;
    std::atomic_bool active;
    
    SoapySDR::Kwargs deviceArgs, streamArgs;
    SoapySDR::Device *soapyDevice;
};
