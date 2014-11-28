#pragma once

#include <vector>
#include <map>

#include "DemodulatorThread.h"

class DemodulatorInstance {
public:
    DemodulatorThread *demodulatorThread;
    std::thread *t_Demod;

    DemodulatorThreadInputQueue* threadQueueDemod;
    DemodulatorThreadCommandQueue* threadQueueCommand;

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
