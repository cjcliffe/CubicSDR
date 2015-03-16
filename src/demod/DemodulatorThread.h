#pragma once

#include <queue>
#include <vector>

#include "DemodDefs.h"
#include "AudioThread.h"

typedef ThreadQueue<AudioThreadInput *> DemodulatorThreadOutputQueue;

#define DEMOD_VIS_SIZE 1024

class DemodulatorThread {
public:

    DemodulatorThread(DemodulatorThreadPostInputQueue* iqInputQueue, DemodulatorThreadControlCommandQueue *threadQueueControl,
            DemodulatorThreadCommandQueue* threadQueueNotify);
    ~DemodulatorThread();

#ifdef __APPLE__
    void *threadMain();
#else
    void threadMain();
#endif

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue);
    void setAudioOutputQueue(AudioThreadInputQueue *tQueue);

    void terminate();

    void setStereo(bool state);
    bool isStereo();

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
    std::deque<AudioThreadInput *> outputBuffers;
    std::deque<AudioThreadInput *>::iterator outputBuffersI;

    std::vector<liquid_float_complex> agcData;
    std::vector<float> agcAMData;
    std::vector<float> demodOutputData;
    std::vector<float> demodStereoData;
    std::vector<float> resampledOutputData;
    std::vector<float> resampledStereoData;

    DemodulatorThreadPostInputQueue* iqInputQueue;
    DemodulatorThreadOutputQueue* audioVisOutputQueue;
    AudioThreadInputQueue *audioOutputQueue;

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

    std::atomic<bool> stereo;
    std::atomic<bool> terminated;
    std::atomic<int> demodulatorType;
    int audioSampleRate;

    DemodulatorThreadCommandQueue* threadQueueNotify;
    DemodulatorThreadControlCommandQueue *threadQueueControl;
    std::atomic<float> squelchLevel;
    std::atomic<float> signalLevel;
    bool squelchEnabled;
};
