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
    std::thread *t_Demod;

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

    void terminateAll();
private:
    std::vector<DemodulatorInstance *> demods;
};
