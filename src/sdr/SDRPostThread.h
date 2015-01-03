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
    void setIQDataOutQueue(DemodulatorThreadInputQueue* iqDataQueue);
    void setIQVisualQueue(DemodulatorThreadInputQueue* iqVisQueue);

    void setNumVisSamples(int num_vis_samples_in);
    int getNumVisSamples();

    void threadMain();
    void terminate();

protected:
    uint32_t sample_rate;

    std::atomic<SDRThreadIQDataQueue *> iqDataInQueue;
    std::atomic<DemodulatorThreadInputQueue *> iqDataOutQueue;
    std::atomic<DemodulatorThreadInputQueue *> iqVisualQueue;

    std::vector<DemodulatorInstance *> demodulators;
    std::vector<DemodulatorInstance *> demodulators_add;
    std::vector<DemodulatorInstance *> demodulators_remove;
    std::atomic<bool> terminated;
    iirfilt_crcf dcFilter;
    int num_vis_samples;
};
