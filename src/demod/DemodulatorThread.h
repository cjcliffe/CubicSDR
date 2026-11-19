// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <queue>
#include <vector>
#include <memory>
#include <cstdint>

#include "DemodDefs.h"
#include "AudioThread.h"
#include "Modem.h"
#include "SpinMutex.h"
#include "SquelchState.h"
#include "MeterBallistics.h"
#include "MeterRange.h"

#define DEMOD_VIS_SIZE 2048
#define DEMOD_SIGNAL_MIN -100
#define DEMOD_SIGNAL_MAX 0

class DemodulatorInstance;

class DemodulatorThread : public IOThread {
public:

    explicit DemodulatorThread(DemodulatorInstance* parent);
    ~DemodulatorThread() override;

    void onBindOutput(std::string name, ThreadQueueBasePtr threadQueue) override;
    
    void run() override;
    void terminate() override;
    
    void setMuted(bool muted_in);
    bool isMuted();
    
    float getSignalLevel();
    float getSignalCeil();
    void setSquelchEnabled(bool squelchEnabled_in);
    void setScaleHold(bool scaleHold_in);
    bool isSquelchEnabled();
    float getSignalFloor();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();
    float getHysteresis();
   
    bool getSquelchBreak();
    void setSquelchBreak(bool squelchBreak_in);
    uint64_t getLastBreakSerial();
protected:
    
    double abMagnitude(float inphase, float quadrature);
    double linearToDb(double linear);

    DemodulatorInstance* demodInstance;
    ReBuffer<AudioThreadInput> outputBuffers;

    std::atomic_bool muted;

    std::atomic<float> squelchLevel;
    std::atomic<float> signalLevel, signalFloor, signalCeil;
    std::atomic<bool> squelchEnabled, squelchBreak, scaleHold;
    std::atomic<float> currentHysteresis;
    std::atomic<uint64_t> lastBreakSerial;
    static std::atomic<uint64_t> breakSerialCounter;

    SquelchState squelch;
    SquelchFade squelchFade;
    MeterBallistics meterBallistics;
    MeterRange meterRange;
    LevelSpread levelSpread;
    
    
    Modem *cModem = nullptr;
    ModemKit *cModemKit = nullptr;
    
    DemodulatorThreadPostInputQueuePtr iqInputQueue;
    AudioThreadInputQueuePtr audioOutputQueue;
    DemodulatorThreadOutputQueuePtr audioVisOutputQueue;
    DemodulatorThreadOutputQueuePtr audioSinkOutputQueue = nullptr;

    //protects the audioVisOutputQueue dynamic binding change at runtime (in DemodulatorMgr)
    SpinMutex m_mutexAudioVisOutputQueue;
};
