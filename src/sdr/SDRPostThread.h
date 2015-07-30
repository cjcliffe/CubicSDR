#pragma once

#include "SDRThread.h"
#include <algorithm>

class SDRPostThread : public IOThread {
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
    
    void setSwapIQ(bool swapIQ);
    bool getSwapIQ();
    
    void run();
    void terminate();

protected:
    std::atomic<SDRThreadIQDataQueue *> iqDataInQueue;
    std::atomic<DemodulatorThreadInputQueue *> iqDataOutQueue;
    std::atomic<DemodulatorThreadInputQueue *> iqVisualQueue;
	
    std::mutex busy_demod;
    std::vector<DemodulatorInstance *> demodulators;
    iirfilt_crcf dcFilter;
    int num_vis_samples;
    std::atomic_bool swapIQ;
    
private:
    std::vector<liquid_float_complex> _lut;
    std::vector<liquid_float_complex> _lut_swap;
};
