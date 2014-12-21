#pragma once

#include <queue>
#include <vector>

#include "CubicSDRDefs.h"
#include "DemodDefs.h"
#include "DemodulatorWorkerThread.h"

class DemodulatorPreThread {
public:

    DemodulatorPreThread(DemodulatorThreadInputQueue* pQueueIn, DemodulatorThreadPostInputQueue* pQueueOut,
            DemodulatorThreadControlCommandQueue *threadQueueControl, DemodulatorThreadCommandQueue* threadQueueNotify);
    ~DemodulatorPreThread();

#ifdef __APPLE__
    void *threadMain();
#else
    void threadMain();
#endif

    void setCommandQueue(DemodulatorThreadCommandQueue *tQueue) {
        commandQueue = tQueue;
    }

    void setAudioInputQueue(AudioThreadInputQueue *tQueue) {
        audioInputQueue = tQueue;
    }

    void setDemodulatorControlQueue(DemodulatorThreadControlCommandQueue *tQueue) {
        threadQueueControl = tQueue;
    }


    DemodulatorThreadParameters &getParams() {
        return params;
    }

    void initialize();

    void terminate();

#ifdef __APPLE__
    static void *pthread_helper(void *context) {
        return ((DemodulatorPreThread *) context)->threadMain();
    }
#endif

protected:
    DemodulatorThreadInputQueue* inputQueue;
    DemodulatorThreadPostInputQueue* postInputQueue;
    DemodulatorThreadCommandQueue* commandQueue;
    AudioThreadInputQueue *audioInputQueue;

    firfilt_crcf fir_filter;
    msresamp_crcf resampler;
    float resample_ratio;

    msresamp_crcf audio_resampler;
    float audio_resample_ratio;

    DemodulatorThreadParameters params;
    DemodulatorThreadParameters last_params;

    freqdem fdem;
    nco_crcf nco_shift;
    int shift_freq;

    std::atomic<bool> terminated;
    std::atomic<bool> initialized;

    DemodulatorWorkerThread *workerThread;
    std::thread *t_Worker;

    DemodulatorThreadWorkerCommandQueue *workerQueue;
    DemodulatorThreadWorkerResultQueue *workerResults;
    DemodulatorThreadCommandQueue* threadQueueNotify;
    DemodulatorThreadControlCommandQueue *threadQueueControl;
};
