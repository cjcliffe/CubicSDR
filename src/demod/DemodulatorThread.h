// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <queue>
#include <vector>
#include <memory>

#include "DemodDefs.h"
#include "AudioThread.h"
#include "Modem.h"
#include "SpinMutex.h"

#define DEMOD_VIS_SIZE 2048
#define DEMOD_SIGNAL_MIN -30
#define DEMOD_SIGNAL_MAX 30

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
    float getSignalFloor();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();
   
    bool getSquelchBreak();


    static void releaseSquelchLock(DemodulatorInstance* inst);
protected:
    
    double abMagnitude(float inphase, float quadrature);
    double linearToDb(double linear);

    DemodulatorInstance* demodInstance;
    ReBuffer<AudioThreadInput> outputBuffers;

    std::atomic_bool muted;

    std::atomic<float> squelchLevel;
    std::atomic<float> signalLevel, signalFloor, signalCeil;
    bool squelchEnabled, squelchBreak;
    
    static DemodulatorInstance* squelchLock;
    static std::mutex squelchLockMutex;
    
    
    Modem *cModem = nullptr;
    ModemKit *cModemKit = nullptr;
    
    DemodulatorThreadPostInputQueuePtr iqInputQueue;
    AudioThreadInputQueuePtr audioOutputQueue;
    DemodulatorThreadOutputQueuePtr audioVisOutputQueue;
    DemodulatorThreadControlCommandQueuePtr threadQueueControl;

    DemodulatorThreadOutputQueuePtr audioSinkOutputQueue = nullptr;

    //protects the audioVisOutputQueue dynamic binding change at runtime (in DemodulatorMgr)
    SpinMutex m_mutexAudioVisOutputQueue;
};
