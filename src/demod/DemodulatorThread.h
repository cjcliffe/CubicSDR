#pragma once

#include <queue>
#include <vector>

#include "DemodDefs.h"
#include "AudioThread.h"

typedef ThreadQueue<AudioThreadInput> DemodulatorThreadOutputQueue;

class DemodulatorThread {
public:

    DemodulatorThread(DemodulatorThreadPostInputQueue* pQueueIn, DemodulatorThreadCommandQueue* threadQueueNotify);
    ~DemodulatorThread();

#ifdef __APPLE__
    void *threadMain();
#else
    void threadMain();
#endif

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
        visOutQueue = tQueue;
    }

    void setAudioInputQueue(AudioThreadInputQueue *tQueue) {
        audioInputQueue = tQueue;
    }

    void initialize();

    void terminate();

#ifdef __APPLE__
    static void *pthread_helper(void *context) {
        return ((DemodulatorThread *) context)->threadMain();
    }
#endif

protected:
    DemodulatorThreadPostInputQueue* postInputQueue;
    DemodulatorThreadOutputQueue* visOutQueue;
    AudioThreadInputQueue *audioInputQueue;

    freqdem fdem;

    std::atomic<bool> terminated;

    DemodulatorThreadCommandQueue* threadQueueNotify;
};
