#pragma once

#include "wx/wxprec.h"
#include "rtl-sdr.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "DemodulatorThreadQueue.h"
#include "DemodulatorMgr.h"
#include "ThreadQueue.h"

class SDRThreadCommand {
public:
    enum SDRThreadCommandEnum {
        SDR_THREAD_CMD_NULL,
        SDR_THREAD_CMD_TUNE
    };

    SDRThreadCommand() : cmd(cmd), int_value(SDR_THREAD_CMD_NULL){

    }

    SDRThreadCommand(SDRThreadCommandEnum cmd) : cmd(cmd), int_value(0) {

    }

    SDRThreadCommandEnum cmd;
    int int_value;
};

class SDRThreadIQData {
public:
    unsigned int frequency;
    unsigned int bandwidth;
    std::vector<signed char> data;

    SDRThreadIQData(): frequency(0), bandwidth(0) {

    }

    SDRThreadIQData(unsigned int bandwidth, unsigned int frequency, std::vector<signed char> data) :
            data(data), frequency(frequency), bandwidth(bandwidth) {

    }

    ~SDRThreadIQData() {

    }
};


typedef ThreadQueue<SDRThreadCommand> SDRThreadCommandQueue;
typedef ThreadQueue<SDRThreadIQData> SDRThreadIQDataQueue;

class SDRThread {
public:
    rtlsdr_dev_t *dev;

    SDRThread(SDRThreadCommandQueue* pQueue);
    ~SDRThread();

    int enumerate_rtl();

    void bindDemodulator(DemodulatorInstance &demod) {
        demodulators.push_back(demod.threadQueueDemod);
    }

    void threadMain();

    void setIQDataOutQueue(SDRThreadIQDataQueue* iqDataQueue) {
        iqDataOutQueue = iqDataQueue;
    }
    void setIQVisualQueue(SDRThreadIQDataQueue *iqVisQueue) {
        iqVisualQueue = iqVisQueue;
        iqVisualQueue->set_max_num_items(1);
    }
protected:

    uint32_t sample_rate;
    SDRThreadCommandQueue* m_pQueue;
    SDRThreadIQDataQueue* iqDataOutQueue;
    SDRThreadIQDataQueue* iqVisualQueue;

    std::vector<DemodulatorThreadQueue *> demodulators;
};
