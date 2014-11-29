#pragma once

#include "SDRThread.h"

class SDRPostThread {
public:
    rtlsdr_dev_t *dev;

    SDRPostThread();
    ~SDRPostThread();

    int enumerate_rtl();

    void bindDemodulator(DemodulatorInstance *demod) {
        demodulators.push_back(demod);
    }

    void threadMain();

    void setIQDataInQueue(SDRThreadIQDataQueue* iqDataQueue) {
        iqDataInQueue = iqDataQueue;
    }
    void setIQDataOutQueue(SDRThreadIQDataQueue* iqDataQueue) {
        iqDataOutQueue = iqDataQueue;
    }
    void setIQVisualQueue(SDRThreadIQDataQueue *iqVisQueue) {
        iqVisualQueue = iqVisQueue;
        iqVisualQueue.load()->set_max_num_items(1);
    }

    void terminate();
protected:

    uint32_t sample_rate;

    std::atomic<SDRThreadIQDataQueue*> iqDataOutQueue;
    std::atomic<SDRThreadIQDataQueue*> iqDataInQueue;
    std::atomic<SDRThreadIQDataQueue*> iqVisualQueue;

    std::vector<DemodulatorInstance *> demodulators;
    std::atomic<bool> terminated;
    iirfilt_crcf dcFilter;
};
