// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <atomic>
#include <memory>
#include "ThreadBlockingQueue.h"
#include "DemodulatorMgr.h"
#include "SDRDeviceInfo.h"
#include "AppConfig.h"

#include <SoapySDR/Version.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Device.hpp>

#include <cstddef>

class SDRThreadIQData {
public:
    long long frequency;
    long long sampleRate;
    bool dcCorrected;
    int numChannels;
    std::vector<liquid_float_complex> data;

    SDRThreadIQData() :
            frequency(0), sampleRate(DEFAULT_SAMPLE_RATE), dcCorrected(true), numChannels(0) {

    }

    SDRThreadIQData(long long bandwidth, long long frequency, std::vector<signed char> * /* data */) :
            frequency(frequency), sampleRate(bandwidth) {

    }

    virtual ~SDRThreadIQData() = default;
};
typedef std::shared_ptr<SDRThreadIQData> SDRThreadIQDataPtr;
typedef ThreadBlockingQueue<SDRThreadIQDataPtr> SDRThreadIQDataQueue;
typedef std::shared_ptr<SDRThreadIQDataQueue> SDRThreadIQDataQueuePtr;

class SDRThread : public IOThread {
private:
    bool init();
    void deinit();
    
    //returns the SoapyDevice readStream return value,
    //i.e if >= 0 the number of samples read, else if < 0 an error code.
    int readStream(const SDRThreadIQDataQueuePtr& iqDataOutQueue);

    void readLoop();

public:
    SDRThread();
    ~SDRThread() override;

    enum SDRThreadState { SDR_THREAD_MESSAGE, SDR_THREAD_INITIALIZED, SDR_THREAD_FAILED};
    
    void run() override;
    void terminate() override;

    SDRDeviceInfo *getDevice();
    void setDevice(SDRDeviceInfo *dev);
    int getOptimalElementCount(long long sampleRate_in, int fps);
    int getOptimalChannelCount(long long sampleRate_in);
    
    void setFrequency(long long freq);
    long long getFrequency();
    
    void lockFrequency(long long freq);
    bool isFrequencyLocked();
    void unlockFrequency();
    
    void setOffset(long long ofs);
    long long getOffset();

    void setAntenna(const std::string& name);
    std::string getAntenna();
    
    void setSampleRate(long rate);
    long getSampleRate();

    void setPPM(int ppm_in);
    int getPPM();
    
    void setAGCMode(bool mode);
    bool getAGCMode();

    void setIQSwap(bool swap);
    bool getIQSwap();

    void setGain(const std::string& name, float value);
    float getGain(const std::string& name);
    
    void writeSetting(const std::string& name, std::string value);
    std::string readSetting(const std::string& name);
    
    void setStreamArgs(SoapySDR::Kwargs streamArgs);
    
protected:
    void updateGains();
    void updateSettings();
    SoapySDR::Kwargs combineArgs(SoapySDR::Kwargs a, SoapySDR::Kwargs b);

    SoapySDR::Stream *stream = nullptr;
    SoapySDR::Device *device;
    void *buffs[1] = { nullptr };
    ReBuffer<SDRThreadIQData> buffers;
    SDRThreadIQData overflowBuffer;
    int numOverflow;
    std::atomic<DeviceConfig *> deviceConfig;
    std::atomic<SDRDeviceInfo *> deviceInfo;
    
    std::mutex setting_busy;
    std::map<std::string, std::string> settings;
    std::map<std::string, bool> settingChanged;

    std::atomic_llong sampleRate;
    std::atomic_llong frequency, offset, lock_freq;
    std::atomic_int ppm, numElems, mtuElems, numChannels;
    std::atomic_bool hasPPM, hasHardwareDC;
    std::string antennaName;
    std::atomic_bool agc_mode, rate_changed, freq_changed, offset_changed, antenna_changed,
        ppm_changed, device_changed, agc_mode_changed, gain_value_changed, setting_value_changed, frequency_locked, frequency_lock_init, iq_swap;

    std::mutex gain_busy;
    std::map<std::string, float> gainValues;
    std::map<std::string, bool> gainChanged;
    
    SoapySDR::Kwargs streamArgs;

private:
	void assureBufferMinSize(SDRThreadIQData * dataOut, size_t minSize);
};
