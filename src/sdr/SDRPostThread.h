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
    void bindDemodulators(std::vector<DemodulatorInstance *> *demods);
    void removeDemodulator(DemodulatorInstance *demod);
    
    void run();
    void terminate();

    void runSingleCH(SDRThreadIQData *data_in);
    void runPFBCH(SDRThreadIQData *data_in);
    void setIQVisualRange(long long frequency, int bandwidth);
        
protected:
    SDRThreadIQDataQueue *iqDataInQueue;
    DemodulatorThreadInputQueue *iqDataOutQueue;
    DemodulatorThreadInputQueue *iqVisualQueue;
    DemodulatorThreadInputQueue *iqActiveDemodVisualQueue;
    
    std::mutex busy_demod;
    std::vector<DemodulatorInstance *> demodulators;

private:
    void initPFBChannelizer();
    void updateActiveDemodulators();
    void updateChannels();
    int getChannelAt(long long frequency);

    ReBuffer<DemodulatorThreadIQData> buffers;
    std::vector<liquid_float_complex> fpData;
    std::vector<liquid_float_complex> dataOut;
    std::vector<long long> chanCenters;
    long long chanBw;
    
    size_t nRunDemods;
    std::vector<DemodulatorInstance *> runDemods;
    std::vector<int> demodChannel;
    std::vector<int> demodChannelActive;

    ReBuffer<DemodulatorThreadIQData> visualDataBuffers;
    atomic_bool doRefresh;
    atomic_llong visFrequency;
    atomic_int visBandwidth;
    int numChannels, sampleRate;
    long long frequency;
    firpfbch_crcf channelizer;
    iirfilt_crcf dcFilter;
    std::vector<liquid_float_complex> dcBuf;
};
