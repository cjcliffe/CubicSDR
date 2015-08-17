#pragma once

#include <queue>
#include <vector>

#include "DemodDefs.h"
#include "AudioThread.h"

typedef ThreadQueue<AudioThreadInput *> DemodulatorThreadOutputQueue;

#define DEMOD_VIS_SIZE 1024

class DemodulatorThread : public IOThread {
public:

    DemodulatorThread();
    ~DemodulatorThread();

    void onBindOutput(std::string name, ThreadQueueBase *threadQueue);
    
    void run();
    void terminate();

    void setStereo(bool state);
    bool isStereo();

    void setAGC(bool state);
    bool getAGC();

    void setMuted(bool state);
    bool isMuted();
    
    float getSignalLevel();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();

    void setDemodulatorType(int demod_type_in);
    int getDemodulatorType();

#ifdef __APPLE__
    static void *pthread_helper(void *context) {
        return ((DemodulatorThread *) context)->threadMain();
    }
#endif

protected:
    ReBuffer<AudioThreadInput> outputBuffers;

    std::vector<liquid_float_complex> agcData;
    std::vector<float> agcAMData;
    std::vector<float> demodOutputData;
    std::vector<float> demodStereoData;
    std::vector<float> resampledOutputData;
    std::vector<float> resampledStereoData;

    freqdem demodFM;
    ampmodem demodAM;
    ampmodem demodAM_DSB_CSP;
    ampmodem demodAM_DSB;
    ampmodem demodAM_LSB;
    ampmodem demodAM_USB;

    agc_crcf iqAutoGain;

    float amOutputCeil;
    float amOutputCeilMA;
    float amOutputCeilMAA;

    std::atomic_bool stereo;
    std::atomic_bool muted;
    std::atomic_bool agcEnabled;
    std::atomic_int demodulatorType;
    int audioSampleRate;

    std::atomic<float> squelchLevel;
    std::atomic<float> signalLevel;
    bool squelchEnabled;
    
    DemodulatorThreadPostInputQueue* iqInputQueue;
    AudioThreadInputQueue *audioOutputQueue;
    DemodulatorThreadOutputQueue* audioVisOutputQueue;
    DemodulatorThreadControlCommandQueue *threadQueueControl;
    DemodulatorThreadCommandQueue* threadQueueNotify;
};
