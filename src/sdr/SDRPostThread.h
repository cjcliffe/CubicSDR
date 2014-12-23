#pragma once

#include "SDRThread.h"
#include <algorithm>

class SDRPostThread {
public:
    SDRPostThread();
    ~SDRPostThread();

    void bindDemodulator(DemodulatorInstance *demod);
    void removeDemodulator(DemodulatorInstance *demod);

    void setIQDataInQueue(SDRThreadIQDataQueue* iqDataQueue);
    void setIQDataOutQueue(SDRThreadIQDataQueue* iqDataQueue);
    void setIQVisualQueue(SDRThreadIQDataQueue *iqVisQueue);

    void threadMain();
    void terminate();

protected:
    uint32_t sample_rate;

    std::atomic<SDRThreadIQDataQueue*> iqDataOutQueue;
    std::atomic<SDRThreadIQDataQueue*> iqDataInQueue;
    std::atomic<SDRThreadIQDataQueue*> iqVisualQueue;

    std::vector<DemodulatorInstance *> demodulators;
    std::vector<DemodulatorInstance *> demodulators_add;
    std::vector<DemodulatorInstance *> demodulators_remove;
    std::atomic<bool> terminated;
    iirfilt_crcf dcFilter;
};
