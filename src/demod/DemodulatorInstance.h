#pragma once

#include <vector>
#include <map>
#include <thread>

#include "DemodulatorThread.h"
#include "DemodulatorPreThread.h"

class DemodulatorInstance {
public:

    DemodulatorThreadInputQueue* threadQueueDemod;
    DemodulatorThreadPostInputQueue* threadQueuePostDemod;
    DemodulatorThreadCommandQueue* threadQueueCommand;
    DemodulatorThreadCommandQueue* threadQueueNotify;
    DemodulatorPreThread *demodulatorPreThread;
    DemodulatorThread *demodulatorThread;
#ifdef __APPLE__
    pthread_t t_PreDemod;
    pthread_t t_Demod;
#else
    std::thread *t_PreDemod;
    std::thread *t_Demod;
#endif

    AudioThreadInputQueue *audioInputQueue;
    AudioThread *audioThread;
    std::thread *t_Audio;

    DemodulatorInstance();
    ~DemodulatorInstance();

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue);

    DemodulatorThreadCommandQueue *getCommandQueue();
    DemodulatorThreadParameters &getParams();

    void run();
    void terminate();
    std::string getLabel();
    void setLabel(std::string labelStr);

    bool isTerminated();
    void updateLabel(int freq);

private:
    std::atomic<std::string *> label;bool terminated;bool demodTerminated;bool audioTerminated;bool preDemodTerminated;
};

