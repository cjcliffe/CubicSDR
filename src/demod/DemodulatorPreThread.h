// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <queue>
#include <vector>
#include <atomic>

#include "CubicSDRDefs.h"
#include "DemodDefs.h"
#include "DemodulatorWorkerThread.h"

class DemodulatorInstance;

class DemodulatorPreThread : public IOThread {
public:

    DemodulatorPreThread(DemodulatorInstance *parent);
    ~DemodulatorPreThread();

    virtual void run();
    
    void setDemodType(std::string demodType);
    std::string getDemodType();

    void setFrequency(long long sampleRate);
    long long getFrequency();

    void setSampleRate(long long sampleRate);
    long long getSampleRate();
    
    void setBandwidth(int bandwidth);
    int getBandwidth();
    
    void setAudioSampleRate(int rate);
    int getAudioSampleRate();
    
    bool isInitialized();
    
    virtual void terminate();

    Modem *getModem();
    ModemKit *getModemKit();
    
    std::string readModemSetting(std::string setting);
    void writeModemSetting(std::string setting, std::string value);
    ModemSettings readModemSettings();
    void writeModemSettings(ModemSettings settings);

protected:
    DemodulatorInstance *parent;
    msresamp_crcf iqResampler;
    double iqResampleRatio;
    std::vector<liquid_float_complex> resampledData;

    Modem *cModem;
    ModemKit *cModemKit;
    
    std::atomic_llong currentSampleRate, newSampleRate;
    std::atomic_llong currentFrequency, newFrequency;
    std::atomic_int currentBandwidth, newBandwidth;
    std::atomic_int currentAudioSampleRate, newAudioSampleRate;

    std::atomic_bool sampleRateChanged, frequencyChanged, bandwidthChanged, audioSampleRateChanged;

    ModemSettings modemSettingsBuffered;
    std::atomic_bool modemSettingsChanged;
    
    nco_crcf freqShifter;
    int shiftFrequency;

    std::atomic_bool initialized;
    std::atomic_bool demodTypeChanged;
    std::string demodType;
    std::string newDemodType;

    DemodulatorWorkerThread *workerThread;
    std::thread *t_Worker;

    DemodulatorThreadWorkerCommandQueue *workerQueue;
    DemodulatorThreadWorkerResultQueue *workerResults;

    DemodulatorThreadInputQueue* iqInputQueue;
    DemodulatorThreadPostInputQueue* iqOutputQueue;
};
