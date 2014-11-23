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

    DemodulatorInstance() :
            t_Demod(NULL), threadQueueDemod(NULL), demodulatorThread(NULL) {
    }
    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
        demodulatorThread->setVisualOutputQueue(tQueue);
    }

    void init() {
        if (threadQueueDemod) {
            delete threadQueueDemod;
        }
        if (demodulatorThread) {
            delete demodulatorThread;
        }

        threadQueueDemod = new DemodulatorThreadInputQueue;
        demodulatorThread = new DemodulatorThread(threadQueueDemod, &params);

        t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
    }

};

class DemodulatorMgr {
public:
    DemodulatorMgr() {

    }

    ~DemodulatorMgr() {
        while (demods.size()) {
            DemodulatorInstance *d = demods.back();
            demods.pop_back();
            delete d;
        }
    }

    DemodulatorInstance *newThread(wxEvtHandler* pParent) {
        DemodulatorInstance *newDemod = new DemodulatorInstance;
        demods.push_back(newDemod);
        return newDemod;
    }

    std::vector<DemodulatorInstance *> demods;
};
