#pragma once

#include <queue>
#include <vector>

#include "DemodDefs.h"
#include "AudioThread.h"
#include "Modem.h"

typedef ThreadQueue<AudioThreadInput *> DemodulatorThreadOutputQueue;

#define DEMOD_VIS_SIZE 2048
#define DEMOD_SIGNAL_MIN -30
#define DEMOD_SIGNAL_MAX 30

class DemodulatorInstance;

class DemodulatorThread : public IOThread {
public:

    DemodulatorThread(DemodulatorInstance *parent);
    ~DemodulatorThread();

    void onBindOutput(std::string name, ThreadQueueBase *threadQueue);
    
    void run();
    void terminate();
    
    void setMuted(bool state);
    bool isMuted();
    
    float getSignalLevel();
    float getSignalCeil();
    float getSignalFloor();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();

    bool getSquelchBreak();

    static void releaseSquelchLock(DemodulatorInstance *inst);
    
protected:
    
    float abMagnitude(double alpha, double beta, float inphase, float quadrature);
    float linearToDb(float linear);

    DemodulatorInstance *demodInstance = nullptr;
    ReBuffer<AudioThreadInput> outputBuffers;

    std::atomic_bool muted;

    std::atomic<float> squelchLevel;
    std::atomic<float> signalLevel, signalFloor, signalCeil;
    bool squelchEnabled, squelchBreak;
    
    static std::atomic<DemodulatorInstance *> squelchLock;
    static std::mutex squelchLockMutex;
    
    
    Modem *cModem = nullptr;
    ModemKit *cModemKit = nullptr;
    
    DemodulatorThreadPostInputQueue* iqInputQueue = nullptr;
    AudioThreadInputQueue *audioOutputQueue = nullptr;
    DemodulatorThreadOutputQueue* audioVisOutputQueue = nullptr;
    DemodulatorThreadControlCommandQueue *threadQueueControl = nullptr;

    //protects the audioVisOutputQueue dynamic binding change at runtime (in DemodulatorMgr)
    mutable std::mutex m_mutexAudioVisOutputQueue;
};
