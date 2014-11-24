#pragma once

#include <vector>
#include <map>

#include "DemodulatorThread.h"

class DemodulatorInstance {
public:
    DemodulatorThread *demodulatorThread;
    std::thread *t_Demod;

    DemodulatorThreadInputQueue* threadQueueDemod;
    DemodulatorThreadParameters params;

    DemodulatorInstance();
    ~DemodulatorInstance();
    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue);
    void init();
    void terminate();
};

class DemodulatorMgr {
public:
    DemodulatorMgr();
    ~DemodulatorMgr();

    DemodulatorInstance *newThread();

    void terminateAll();
private:
    std::vector<DemodulatorInstance *> demods;
};
