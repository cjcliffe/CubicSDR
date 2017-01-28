// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "SoapySDRThread.h"
#include <algorithm>

class SDRPostThread : public IOThread {
public:
    SDRPostThread();
    ~SDRPostThread();

    void bindDemodulator(DemodulatorInstance *demod);
    void bindDemodulators(std::vector<DemodulatorInstance *> *demods);
    void removeDemodulator(DemodulatorInstance *demod);
    
    virtual void run();
    virtual void terminate();

    void runSingleCH(SDRThreadIQData *data_in);
    void runPFBCH(SDRThreadIQData *data_in);
    void setIQVisualRange(long long frequency, int bandwidth);
        
protected:
    SDRThreadIQDataQueue *iqDataInQueue;
    DemodulatorThreadInputQueue *iqDataOutQueue;
    DemodulatorThreadInputQueue *iqVisualQueue;
    DemodulatorThreadInputQueue *iqActiveDemodVisualQueue;
    
    //protects access to demodulators lists and such
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
    long long chanBw = 0;
    
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
