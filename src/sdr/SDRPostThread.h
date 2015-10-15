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
    std::atomic_bool swapIQ;

private:
    void initPFBChannelizer();
    void updateActiveDemodulators();
    void updateChannels();

    ReBuffer<DemodulatorThreadIQData> buffers;
    std::vector<liquid_float_complex> fpData;
    std::vector<liquid_float_complex> dataOut;
    std::vector<long long> chanCenters;
    long long chanBw;
    
    int nRunDemods;
    std::vector<DemodulatorInstance *> runDemods;
    std::vector<int> demodChannel;
    std::vector<int> demodChannelActive;

    ReBuffer<DemodulatorThreadIQData> visualDataBuffers;
    atomic_bool doRefresh;
    int numChannels, sampleRate;
    long long frequency;
    firpfbch2_crcf channelizer;
};
