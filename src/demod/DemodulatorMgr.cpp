#include <DemodulatorMgr.h>

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), threadQueueDemod(NULL), demodulatorThread(NULL) {

    threadQueueDemod = new DemodulatorThreadInputQueue;
    threadQueueCommand = new DemodulatorThreadCommandQueue;
    demodulatorThread = new DemodulatorThread(threadQueueDemod);
    demodulatorThread->setCommandQueue(threadQueueCommand);

}

DemodulatorInstance::~DemodulatorInstance() {
    delete threadQueueDemod;
    delete demodulatorThread;
    delete t_Demod;
}

void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setVisualOutputQueue(tQueue);
}

void DemodulatorInstance::run() {
    if (t_Demod) {
        terminate();
        delete threadQueueDemod;
        delete demodulatorThread;
        delete t_Demod;

        threadQueueDemod = new DemodulatorThreadInputQueue;
        threadQueueCommand = new DemodulatorThreadCommandQueue;
        demodulatorThread = new DemodulatorThread(threadQueueDemod);
        demodulatorThread->setCommandQueue(threadQueueCommand);
    }

    t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
}

DemodulatorThreadCommandQueue *DemodulatorInstance::getCommandQueue() {
    return threadQueueCommand;
}

DemodulatorThreadParameters &DemodulatorInstance::getParams() {
    return demodulatorThread->getParams();
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

std::vector<DemodulatorInstance *> &DemodulatorMgr::getDemodulators() {
    return demods;
}
