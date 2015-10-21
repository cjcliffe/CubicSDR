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
    SDRDeviceRange();
    SDRDeviceRange(double low, double high);
    
    double getLow();
    void setLow(double low);
    double getHigh();
    void setHigh(double high);
    
private:
    double low, high;
};

class SDRDeviceChannel {
public:
    SDRDeviceChannel();
    ~SDRDeviceChannel();
    
    int getChannel();
    void setChannel(int channel);
    
    bool isFullDuplex();
    void setFullDuplex(bool fullDuplex);
    
    bool isTx();
    void setTx(bool tx);
    
    bool isRx();
    void setRx(bool rx);
    
    SDRDeviceRange &getGain();
    SDRDeviceRange &getLNAGain();
    SDRDeviceRange &getFreqRange();
    SDRDeviceRange &getRFRange();

    std::vector<long> &getSampleRates();
    long getSampleRateNear(long sampleRate_in);
    std::vector<long long> &getFilterBandwidths();
    
    const bool& hasHardwareDC() const;
    void setHardwareDC(const bool& hardware);

    const bool& hasCORR() const;
    void setCORR(const bool& corr);
    
    
private:
    int channel;
    bool fullDuplex, tx, rx, hardwareDC, hasCorr;
    SDRDeviceRange rangeGain, rangeLNA, rangeFull, rangeRF;
    std::vector<long> sampleRates;
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
    
    void addChannel(SDRDeviceChannel *chan);
    std::vector<SDRDeviceChannel *> &getChannels();
    SDRDeviceChannel * getRxChannel();
    SDRDeviceChannel * getTxChannel();
    
    void setDeviceArgs(SoapySDR::Kwargs deviceArgs);
    SoapySDR::Kwargs getDeviceArgs();

    void setStreamArgs(SoapySDR::Kwargs deviceArgs);
    SoapySDR::Kwargs getStreamArgs();

private:
    int index;
    std::string name, serial, product, manufacturer, tuner;
    std::string driver, hardware;
    bool timestamps, available;
    
    SoapySDR::Kwargs deviceArgs, streamArgs;
    std::vector<SDRDeviceChannel *> channels;
};
