#pragma once

#include <vector>
#include <map>
#include <thread>

#include "DemodulatorThread.h"

class DemodulatorInstance {
public:

    DemodulatorThreadInputQueue* threadQueueDemod;
    DemodulatorThreadCommandQueue* threadQueueCommand;
    DemodulatorThread *demodulatorThread;
#ifdef __APPLE__
    pthread_t t_Demod;
#else
    std::thread *t_Demod;
#endif

    AudioThreadInputQueue *audioInputQueue;
    AudioThread *audioThread;
    std::thread *t_Audio;

    DemodulatorInstance();
    ~DemodulatorInstance();

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue);

    DemodulatorThreadCommandQueue *getCommandQueue();
    DemodulatorThreadParameters &getParams();

    void run();
    void terminate();
};

class DemodulatorMgr {
public:
    DemodulatorMgr();
    ~DemodulatorMgr();

    DemodulatorInstance *newThread();
    std::vector<DemodulatorInstance *> &getDemodulators();
    std::vector<DemodulatorInstance *> *getDemodulatorsAt(int freq, int bandwidth);

    void terminateAll();
private:
    std::vector<DemodulatorInstance *> demods;
};
