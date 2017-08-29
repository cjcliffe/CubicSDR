// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "SoapySDRThread.h"
#include <algorithm>

class SDRPostThread : public IOThread {
public:
    SDRPostThread();
    ~SDRPostThread();

    void notifyDemodulatorsChanged();
   
    virtual void run();
    virtual void terminate();

    void runSingleCH(SDRThreadIQData *data_in);
    void runPFBCH(SDRThreadIQData *data_in);
    void setIQVisualRange(long long frequency, int bandwidth);
        
protected:
    SDRThreadIQDataQueuePtr iqDataInQueue;
    DemodulatorThreadInputQueuePtr iqDataOutQueue;
    DemodulatorThreadInputQueuePtr iqVisualQueue;
    DemodulatorThreadInputQueuePtr iqActiveDemodVisualQueue;

private:

    void initPFBChannelizer();
    void updateActiveDemodulators();
    void updateChannels();
    int getChannelAt(long long frequency);

    void resetAllDemodulators();

    ReBuffer<DemodulatorThreadIQData> buffers;
    std::vector<liquid_float_complex> fpData;
    std::vector<liquid_float_complex> dataOut;
    std::vector<long long> chanCenters;
    long long chanBw = 0;
    
    std::vector<DemodulatorInstancePtr> runDemods;
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
