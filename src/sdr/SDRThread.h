#pragma once

#include <atomic>

#include "wx/wxprec.h"
#include "rtl-sdr.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "ThreadQueue.h"
#include "DemodulatorMgr.h"

class SDRThreadCommand {
public:
    enum SDRThreadCommandEnum {
        SDR_THREAD_CMD_NULL, SDR_THREAD_CMD_TUNE, SDR_THREAD_CMD_SET_OFFSET, SDR_THREAD_CMD_SET_SAMPLERATE
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
    std::vector<signed char> data;

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

class SDRThread {
public:
    rtlsdr_dev_t *dev;

    SDRThread(SDRThreadCommandQueue* pQueue);
    ~SDRThread();

    int enumerate_rtl();

    void threadMain();

    void setIQDataOutQueue(SDRThreadIQDataQueue* iqDataQueue) {
        iqDataOutQueue = iqDataQueue;
    }

    void terminate();
protected:
    uint32_t sampleRate;
    long long offset;
    std::atomic<SDRThreadCommandQueue*> commandQueue;
    std::atomic<SDRThreadIQDataQueue*> iqDataOutQueue;

    std::atomic<bool> terminated;
};
