#pragma once

#include "SDRThread.h"
#include <algorithm>

class SDRPostThread {
public:
    rtlsdr_dev_t *dev;

    SDRPostThread();
    ~SDRPostThread();

    int enumerate_rtl();

    void bindDemodulator(DemodulatorInstance *demod) {
        demodulators.push_back(demod);
    }

    void removeDemodulator(DemodulatorInstance *demod) {
        if (!demod) {
            return;
        }

        std::vector<DemodulatorInstance *>::iterator i;

        i = std::find(demodulators.begin(), demodulators.end(), demod);

        if (i != demodulators.end()) {
            demodulators.erase(i);
        }
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
