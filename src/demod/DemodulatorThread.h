#pragma once

#include <queue>
#include <vector>

#include "DemodDefs.h"
#include "AudioThread.h"
#include "Modem.h"

typedef ThreadQueue<AudioThreadInput *> DemodulatorThreadOutputQueue;

#define DEMOD_VIS_SIZE 2048
class DemodulatorInstance;

class DemodulatorThread : public IOThread {
public:

    DemodulatorThread(DemodulatorInstance *parent);
    ~DemodulatorThread();

    void onBindOutput(std::string name, ThreadQueueBase *threadQueue);
    
    void run();
    void terminate();
    
    void setAGC(bool state);
    bool getAGC();

    void setMuted(bool state);
    bool isMuted();
    
    float getSignalLevel();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();

//
//#ifdef __APPLE__
//    static void *pthread_helper(void *context) {
//        return ((DemodulatorThread *) context)->threadMain();
//    }
//#endif

protected:
    DemodulatorInstance *demodInstance;
    ReBuffer<AudioThreadInput> outputBuffers;

    std::vector<liquid_float_complex> agcData;
    std::vector<float> agcAMData;

    agc_crcf iqAutoGain;

    std::atomic_bool muted;
    std::atomic_bool agcEnabled;
    int audioSampleRate;

    std::atomic<float> squelchLevel;
    std::atomic<float> signalLevel;
    bool squelchEnabled;
    
    Modem *cModem;
    ModemKit *cModemKit;
    
    DemodulatorThreadPostInputQueue* iqInputQueue;
    AudioThreadInputQueue *audioOutputQueue;
    DemodulatorThreadOutputQueue* audioVisOutputQueue;
    DemodulatorThreadControlCommandQueue *threadQueueControl;
    DemodulatorThreadCommandQueue* threadQueueNotify;
};
