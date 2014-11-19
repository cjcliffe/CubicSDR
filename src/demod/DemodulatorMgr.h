#pragma once

#include <vector>
#include <map>

#include "DemodulatorThread.h"
#include "DemodulatorThreadQueue.h"
#include "DemodulatorThreadTask.h"

class DemodulatorInstance {
public:
    DemodulatorThread *t_Demod;
    DemodulatorThreadQueue* threadQueueDemod;
    DemodulatorThreadParameters params;
    wxEvtHandler* parent;

    DemodulatorInstance(wxEvtHandler* pParent) :
            t_Demod(NULL), threadQueueDemod(NULL), parent(pParent) {
    }

    void init() {
        threadQueueDemod = new DemodulatorThreadQueue(parent);
        t_Demod = new DemodulatorThread(threadQueueDemod, &params);
    }

    void addTask(const DemodulatorThreadTask& task, const DemodulatorThreadQueue::DEMOD_PRIORITY& priority) {
        threadQueueDemod->addTask(task, priority);
    }

    bool run() {
        init();
        if (t_Demod->Run() != wxTHREAD_NO_ERROR) {
            wxLogError
            ("Can't create the Demodulator thread!");
            delete t_Demod;
            delete threadQueueDemod;
            t_Demod = NULL;
            threadQueueDemod = NULL;
            return false;
        }
        t_Demod->SetPriority(80);

        return true;
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
        DemodulatorInstance *newDemod = new DemodulatorInstance(pParent);
        demods.push_back(newDemod);
        return newDemod;
    }

    std::vector<DemodulatorInstance *> demods;
};
