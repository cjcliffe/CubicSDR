#pragma once

#include <atomic>

#include "ThreadQueue.h"
#include "DemodulatorMgr.h"
#include "SDRDeviceInfo.h"
#include "AppConfig.h"

#include <SoapySDR/Version.hpp>
#include <SoapySDR/Modules.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Device.hpp>


class SDRThreadIQData: public ReferenceCounter {
public:
    long long frequency;
    long long sampleRate;
    bool dcCorrected;
    std::vector<float> data;

    SDRThreadIQData() :
            frequency(0), sampleRate(DEFAULT_SAMPLE_RATE), dcCorrected(true) {

    }

    SDRThreadIQData(long long bandwidth, long long frequency, std::vector<signed char> *data) :
            frequency(frequency), sampleRate(bandwidth) {

    }

    ~SDRThreadIQData() {

    }
};

typedef ThreadQueue<SDRThreadIQData *> SDRThreadIQDataQueue;

class SDRThread : public IOThread {
private:
    void init();
    void deinit();
    void readStream(SDRThreadIQDataQueue* iqDataOutQueue);
    void readLoop();

public:
    SDRThread();
    ~SDRThread();
    enum SDRThreadState { SDR_THREAD_DEVICES_READY, SDR_THREAD_NO_DEVICES, SDR_THREAD_TERMINATED, SDR_THREAD_FAILED };
    
    static std::vector<SDRDeviceInfo *> *enumerate_devices();

    void run();

    SDRDeviceInfo *getDevice();
    void setDevice(SDRDeviceInfo *dev);
    int getOptimalElementCount(long long sampleRate, int fps);
    
    void setFrequency(long long freq);
    long long getFrequency();
    
    void setOffset(long long ofs);
    long long getOffset();
    
    void setSampleRate(int rate);
    int getSampleRate();

    void setPPM(int ppm);
    int getPPM();
    
    void setDirectSampling(int dsMode);
    int getDirectSampling();
 
protected:
    static std::vector<std::string> factories;
    static std::vector<std::string> modules;
    static std::vector<SDRDeviceInfo *> devs;
    SoapySDR::Stream *stream;
    SoapySDR::Device *device;
    void *buffs[1];
    ReBuffer<SDRThreadIQData> buffers;

    std::atomic<DeviceConfig *> deviceConfig;
    std::atomic<SDRDeviceInfo *> deviceInfo;

    std::atomic<uint32_t> sampleRate;
    std::atomic_llong frequency, offset;
    std::atomic_int ppm, direct_sampling_mode, numElems;
    std::atomic_bool hasPPM, hasHardwareDC;

    std::atomic_bool rate_changed, freq_changed, offset_changed,
        ppm_changed, direct_sampling_changed, device_changed;
};
