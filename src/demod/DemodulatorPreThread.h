#pragma once

#include <queue>
#include <vector>

#include "CubicSDRDefs.h"
#include "DemodDefs.h"
#include "DemodulatorWorkerThread.h"

class DemodulatorPreThread {
public:

    DemodulatorPreThread(DemodulatorThreadInputQueue* iqInputQueue, DemodulatorThreadPostInputQueue* iqOutputQueue,
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
    DemodulatorThreadInputQueue* iqInputQueue;
    DemodulatorThreadPostInputQueue* iqOutputQueue;
    DemodulatorThreadCommandQueue* commandQueue;

    msresamp_crcf iqResampler;
    double iqResampleRatio;

    msresamp_rrrf audioResampler;
    msresamp_rrrf stereoResampler;
    double audioResampleRatio;

    DemodulatorThreadParameters params;
    DemodulatorThreadParameters lastParams;

    nco_crcf freqShifter;
    int shiftFrequency;

    std::atomic<bool> terminated;
    std::atomic<bool> initialized;

    DemodulatorWorkerThread *workerThread;
    std::thread *t_Worker;

    DemodulatorThreadWorkerCommandQueue *workerQueue;
    DemodulatorThreadWorkerResultQueue *workerResults;
    DemodulatorThreadCommandQueue* threadQueueNotify;
    DemodulatorThreadControlCommandQueue *threadQueueControl;
};
