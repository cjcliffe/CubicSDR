#include <DemodulatorMgr.h>

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), threadQueueDemod(NULL), demodulatorThread(NULL) {
}
void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setVisualOutputQueue(tQueue);
}

void DemodulatorInstance::init() {
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

DemodulatorMgr::DemodulatorMgr() {

}

DemodulatorMgr::~DemodulatorMgr() {
    while (demods.size()) {
        DemodulatorInstance *d = demods.back();
        demods.pop_back();
        delete d;
    }
}

DemodulatorInstance *DemodulatorMgr::newThread() {
    DemodulatorInstance *newDemod = new DemodulatorInstance;
    demods.push_back(newDemod);
    return newDemod;
}
