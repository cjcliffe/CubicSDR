// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <queue>
#include <vector>
#include <atomic>
#include <memory>

#include "CubicSDRDefs.h"
#include "DemodDefs.h"
#include "DemodulatorWorkerThread.h"

class DemodulatorInstance;

class DemodulatorPreThread : public IOThread {
public:

    explicit DemodulatorPreThread(DemodulatorInstance* parent);
    ~DemodulatorPreThread() override;

    void run() override;
    
    void setDemodType(std::string demodType_in);
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
    
    void terminate() override;

    Modem *getModem();
    ModemKit *getModemKit();
    
    std::string readModemSetting(const std::string& setting);
    void writeModemSetting(const std::string& setting, std::string value);
    ModemSettings readModemSettings();
    void writeModemSettings(ModemSettings settings);

protected:
  
    DemodulatorInstance* parent;

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

    DemodulatorThreadWorkerCommandQueuePtr workerQueue;
    DemodulatorThreadWorkerResultQueuePtr  workerResults;

    DemodulatorThreadInputQueuePtr iqInputQueue;
    DemodulatorThreadPostInputQueuePtr iqOutputQueue;
};
