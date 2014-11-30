#include <DemodulatorMgr.h>

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), t_Audio(NULL), threadQueueDemod(NULL), demodulatorThread(NULL) {

    threadQueueDemod = new DemodulatorThreadInputQueue;
    threadQueueCommand = new DemodulatorThreadCommandQueue;
    demodulatorThread = new DemodulatorThread(threadQueueDemod);
    demodulatorThread->setCommandQueue(threadQueueCommand);
    audioInputQueue = new AudioThreadInputQueue;
    audioThread = new AudioThread(audioInputQueue);
    demodulatorThread->setAudioInputQueue(audioInputQueue);
}

DemodulatorInstance::~DemodulatorInstance() {

    delete audioThread;
    delete t_Audio;

    delete audioInputQueue;
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
        delete audioThread;
        delete audioInputQueue;
        delete t_Audio;

        threadQueueDemod = new DemodulatorThreadInputQueue;
        threadQueueCommand = new DemodulatorThreadCommandQueue;
        demodulatorThread = new DemodulatorThread(threadQueueDemod);
        demodulatorThread->setCommandQueue(threadQueueCommand);

        audioInputQueue = new AudioThreadInputQueue;
        audioThread = new AudioThread(audioInputQueue);

        demodulatorThread->setAudioInputQueue(audioInputQueue);
    }

    t_Audio = new std::thread(&AudioThread::threadMain, audioThread);
    t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
}

DemodulatorThreadCommandQueue *DemodulatorInstance::getCommandQueue() {
    return threadQueueCommand;
}

DemodulatorThreadParameters &DemodulatorInstance::getParams() {
    return demodulatorThread->getParams();
}

void DemodulatorInstance::terminate() {
    std::cout << "Terminating demodulator thread.." << std::endl;
    demodulatorThread->terminate();
    t_Demod->join();

    std::cout << "Terminating demodulator audio thread.." << std::endl;
    audioThread->terminate();
    t_Audio->join();
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
