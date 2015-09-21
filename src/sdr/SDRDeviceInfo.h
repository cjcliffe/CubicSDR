#pragma once

#include <string>
#include <vector>

#include <SoapySDR/Types.hpp>

/*
    ----------------------------------------------------
    -- Device identification
    ----------------------------------------------------
    driver=rtl
    hardware=rtl

    ----------------------------------------------------
    -- Peripheral summary
    ----------------------------------------------------
    Channels: 1 Rx, 0 Tx
    Timestamps: NO

    ----------------------------------------------------
    -- RX Channel 0
    ----------------------------------------------------
    Full-duplex: YES
    Antennas: RX
    Full gain range: [0, 49.6] dB
    LNA gain range: [0, 49.6] dB
    Full freq range: [24, 1766] MHz
    RF freq range: [24, 1766] MHz
    CORR freq range:  MHz
    Sample rates: [0.25, 2.56] MHz
    Filter bandwidths: [] MHz
*/

class SDRDeviceRange {
public:
    SDRDeviceRange(float low, float high);
    
    float getLow() const;
    void setLow(const float low);
    float getHigh() const;
    void setHigh(const float high);
    
private:
    float low, high;
};

class SDRDeviceChannel {
    int getChannel() const;
    void setChannel(const int channel);
    
    bool isFullDuplex();
    void setFullDuplex(bool fullDuplex);
    
    bool isTx();
    void setTx(bool tx);
    
    bool isRx();
    void setRx(bool rx);
    
    const SDRDeviceRange &getGain() const;
    const SDRDeviceRange &getLNAGain() const;
    const SDRDeviceRange &getFreqRange() const;
    const SDRDeviceRange &getRFRange() const;

    const std::vector<long long> &getSampleRates() const;
    const std::vector<long long> &getFilterBandwidths() const;
    
private:
    int channel;
    bool fullDuplex, tx, rx;
    SDRDeviceRange rangeGain, rangeLNA, rangeFull, rangeRF;
    std::vector<long long> sampleRates;
    std::vector<long long> filterBandwidths;
};


class SDRDeviceInfo {
public:
    SDRDeviceInfo();
    
    std::string getDeviceId();
    
    const int getIndex() const;
    void setIndex(const int index);
    
    bool isAvailable() const;
    void setAvailable(bool available);
    
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
    
    const std::vector<SDRDeviceChannel>& getChannels() const;
    
    void setDeviceArgs(SoapySDR::Kwargs deviceArgs);
    SoapySDR::Kwargs getDeviceArgs();
private:
    int index;
    std::string name, serial, product, manufacturer, tuner;
    std::string driver, hardware;
    bool timestamps, available;
    
    SoapySDR::Kwargs deviceArgs;
    std::vector<SDRDeviceChannel> channels;
};
