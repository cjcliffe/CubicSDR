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
    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue);
    void init();
};

class DemodulatorMgr {
public:
    DemodulatorMgr();
    ~DemodulatorMgr();

    DemodulatorInstance *newThread();

private:
    std::vector<DemodulatorInstance *> demods;
};
