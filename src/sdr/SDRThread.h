#pragma once

#include <atomic>

#include "rtl-sdr.h"

#include "ThreadQueue.h"
#include "DemodulatorMgr.h"
#include "SDRDeviceInfo.h"

class SDRThreadCommand {
public:
    enum SDRThreadCommandEnum {
        SDR_THREAD_CMD_NULL, SDR_THREAD_CMD_TUNE, SDR_THREAD_CMD_SET_OFFSET, SDR_THREAD_CMD_SET_SAMPLERATE, SDR_THREAD_CMD_SET_PPM, SDR_THREAD_CMD_SET_DEVICE, SDR_THREAD_CMD_SET_DIRECT_SAMPLING
    };

    SDRThreadCommand() :
            cmd(SDR_THREAD_CMD_NULL), llong_value(0) {

    }

    SDRThreadCommand(SDRThreadCommandEnum cmd) :
            cmd(cmd), llong_value(0) {

    }

    SDRThreadCommandEnum cmd;
    long long llong_value;
};

class SDRThreadIQData: public ReferenceCounter {
public:
    long long frequency;
    long long sampleRate;
    std::vector<unsigned char> data;

    SDRThreadIQData() :
            frequency(0), sampleRate(DEFAULT_SAMPLE_RATE) {

    }

    SDRThreadIQData(long long bandwidth, long long frequency, std::vector<signed char> *data) :
            frequency(frequency), sampleRate(bandwidth) {

    }

    ~SDRThreadIQData() {

    }
};

typedef ThreadQueue<SDRThreadCommand> SDRThreadCommandQueue;
typedef ThreadQueue<SDRThreadIQData *> SDRThreadIQDataQueue;

class SDRThread : public IOThread {
public:
    rtlsdr_dev_t *dev;

    SDRThread();
    ~SDRThread();

    static int enumerate_rtl(std::vector<SDRDeviceInfo *> *devs);

    void run();
    
    int getDeviceId() const;
    void setDeviceId(int deviceId);

protected:
    std::atomic<uint32_t> sampleRate;
    std::atomic_llong offset;
    std::atomic_int deviceId;
};
