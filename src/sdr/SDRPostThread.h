#pragma once

#if USE_RTL_SDR
#include "SDRThread.h"
#else
#include "SoapySDRThread.h"
#endif
#include <algorithm>

class SDRPostThread : public IOThread {
public:
    SDRPostThread();
    ~SDRPostThread();

    void bindDemodulator(DemodulatorInstance *demod);
    void removeDemodulator(DemodulatorInstance *demod);

    void setSwapIQ(bool swapIQ);
    bool getSwapIQ();
    
    void run();
    void terminate();

protected:
    SDRThreadIQDataQueue *iqDataInQueue;
    DemodulatorThreadInputQueue *iqDataOutQueue;
    DemodulatorThreadInputQueue *iqVisualQueue;
	
    std::mutex busy_demod;
    std::vector<DemodulatorInstance *> demodulators;
    iirfilt_crcf dcFilter;
    std::atomic_bool swapIQ;

    ReBuffer<DemodulatorThreadIQData> visualDataBuffers;
    int numChannels, sampleRate;
    firpfbch2_crcf channelizer;
};
