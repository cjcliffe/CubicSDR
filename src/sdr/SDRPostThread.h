// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "SoapySDRThread.h"
#include <algorithm>

enum SDRPostThreadChannelizerType {
    SDRPostPFBCH = 1,
    SDRPostPFBCH2 = 2
};

class SDRPostThread : public IOThread {
public:
    SDRPostThread();
    ~SDRPostThread() override;

    void notifyDemodulatorsChanged();
   
    void run() override;
    void terminate() override;

    void resetAllDemodulators();

    void setChannelizerType(SDRPostThreadChannelizerType chType);
    SDRPostThreadChannelizerType getChannelizerType();
    
    
protected:
    SDRThreadIQDataQueuePtr iqDataInQueue;
    DemodulatorThreadInputQueuePtr iqDataOutQueue;
    DemodulatorThreadInputQueuePtr iqVisualQueue;
    DemodulatorThreadInputQueuePtr iqActiveDemodVisualQueue;

private:
    // Copy the full samplerate into a new DemodulatorThreadIQDataPtr.
    DemodulatorThreadIQDataPtr getFullSampleRateIqData(SDRThreadIQData *data_in);
    void pushVisualData(const DemodulatorThreadIQDataPtr& iqDataOut);

    void runSingleCH(SDRThreadIQData *data_in);

    void runDemodChannels(int channelBandwidth);

    void initPFBCH();
    void runPFBCH(SDRThreadIQData *data_in);

    void initPFBCH2();
    void runPFBCH2(SDRThreadIQData *data_in);

    void updateActiveDemodulators();
    void updateChannels();    
    int getChannelAt(long long frequency_in);

    ReBuffer<DemodulatorThreadIQData> buffers;
    std::vector<liquid_float_complex> dataOut;
    std::vector<long long> chanCenters;
    long long chanBw = 0;
    
    std::vector<DemodulatorInstancePtr> runDemods;
    std::vector<int> demodChannel;
    std::vector<int> demodChannelActive;

    ReBuffer<DemodulatorThreadIQData> visualDataBuffers;
    atomic_bool doRefresh;
    atomic_int chanMode;

    int numChannels, sampleRate, lastChanMode;
    long long frequency;
    firpfbch_crcf channelizer;
    firpfbch2_crcf channelizer2;
    iirfilt_crcf dcFilter;
    std::vector<liquid_float_complex> dcBuf;
};
