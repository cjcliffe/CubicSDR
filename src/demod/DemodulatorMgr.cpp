#include <DemodulatorMgr.h>

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), threadQueueDemod(NULL), demodulatorThread(NULL) {
}

DemodulatorInstance::~DemodulatorInstance() {
    delete threadQueueDemod;
    delete demodulatorThread;
    delete t_Demod;
}

void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setVisualOutputQueue(tQueue);
}

void DemodulatorInstance::init() {
    if (demodulatorThread) {
        terminate();
        delete threadQueueDemod;
        delete demodulatorThread;
        delete t_Demod;
    }

    threadQueueDemod = new DemodulatorThreadInputQueue;
    demodulatorThread = new DemodulatorThread(threadQueueDemod, &params);

    t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
}

void DemodulatorInstance::terminate() {
    demodulatorThread->terminate();
    t_Demod->join();
}

DemodulatorMgr::DemodulatorMgr() {

}

DemodulatorMgr::~DemodulatorMgr() {
    terminateAll();
}

DemodulatorInstance *DemodulatorMgr::newThread() {
    DemodulatorInstance *newDemod = new DemodulatorInstance;
    demods.push_back(newDemod);
    return newDemod;
}

void DemodulatorMgr::terminateAll() {
    while (demods.size()) {
        DemodulatorInstance *d = demods.back();
        demods.pop_back();
        d->terminate();
        delete d;
    }
}
