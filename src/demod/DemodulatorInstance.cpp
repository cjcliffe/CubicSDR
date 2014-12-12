#include "DemodulatorInstance.h"

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), t_Audio(NULL), threadQueueDemod(NULL), demodulatorThread(NULL), terminated(false), audioTerminated(false), demodTerminated(
        false) {

    label = new std::string("Unnamed");
    threadQueueDemod = new DemodulatorThreadInputQueue;
    threadQueueCommand = new DemodulatorThreadCommandQueue;
    threadQueueNotify = new DemodulatorThreadCommandQueue;
    demodulatorThread = new DemodulatorThread(threadQueueDemod, threadQueueNotify);
    demodulatorThread->setCommandQueue(threadQueueCommand);
    audioInputQueue = new AudioThreadInputQueue;
    audioThread = new AudioThread(audioInputQueue, threadQueueNotify);
    demodulatorThread->setAudioInputQueue(audioInputQueue);
}

DemodulatorInstance::~DemodulatorInstance() {
    delete audioThread;
    delete demodulatorThread;

    delete audioInputQueue;
    delete threadQueueDemod;
}

void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setVisualOutputQueue(tQueue);
}

void DemodulatorInstance::run() {
    t_Audio = new std::thread(&AudioThread::threadMain, audioThread);

#ifdef __APPLE__    // Already using pthreads, might as well do some custom init..
    pthread_attr_t attr;
    size_t size;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2048000);
    pthread_attr_getstacksize(&attr, &size);
    pthread_create(&t_Demod, &attr, &DemodulatorThread::pthread_helper, demodulatorThread);
    pthread_attr_destroy(&attr);

    std::cout << "Initialized demodulator stack size of " << size << std::endl;

#else
    t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
#endif
}

void DemodulatorInstance::updateLabel(int freq) {
    std::stringstream newLabel;
    newLabel.precision(3);
    newLabel << std::fixed << ((float) freq / 1000000.0);
    setLabel(newLabel.str());
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
//#ifdef __APPLE__
//    pthread_join(t_Demod,NULL);
//#else
//#endif
    std::cout << "Terminating demodulator audio thread.." << std::endl;
    audioThread->terminate();
}

std::string DemodulatorInstance::getLabel() {
    return *(label.load());
}

void DemodulatorInstance::setLabel(std::string labelStr) {
    std::string *newLabel = new std::string;
    newLabel->append(labelStr);
    std::string *oldLabel;
    oldLabel = label;
    label = newLabel;
    delete oldLabel;
}
