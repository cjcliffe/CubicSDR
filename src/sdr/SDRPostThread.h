// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "SoapySDRThread.h"
#include <algorithm>

enum SDRPostThreadChannelizerType {
    SDRPostThreadCh = 1,
    SDRPostThreadCh2 = 2
};

class SDRPostThread : public IOThread {
public:
    SDRPostThread();
    ~SDRPostThread();

    void notifyDemodulatorsChanged();
   
    virtual void run();
    virtual void terminate();

    void pushVisualData(SDRThreadIQData *data_in);
    void runSingleCH(SDRThreadIQData *data_in);
    void runPFBCH(SDRThreadIQData *data_in);
    void runPFBCH2(SDRThreadIQData *data_in);
    void setIQVisualRange(long long frequency, int bandwidth);
    void setChannelizerType(SDRPostThreadChannelizerType chType);
    SDRPostThreadChannelizerType getChannelizerType();
    
    
protected:
    SDRThreadIQDataQueuePtr iqDataInQueue;
    DemodulatorThreadInputQueuePtr iqDataOutQueue;
    DemodulatorThreadInputQueuePtr iqVisualQueue;
    DemodulatorThreadInputQueuePtr iqActiveDemodVisualQueue;

private:

    void initPFBChannelizer();
    void initPFBChannelizer2();
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
    atomic_int chanMode;

    int numChannels, sampleRate, lastChanMode;
    long long frequency;
    firpfbch_crcf channelizer;
    firpfbch2_crcf channelizer2;
    iirfilt_crcf dcFilter;
    std::vector<liquid_float_complex> dcBuf;
};
