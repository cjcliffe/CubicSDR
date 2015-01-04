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
    DemodulatorThreadControlCommandQueue *threadQueueControl;

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
    void updateLabel(long long freq);

    bool isActive();
    void setActive(bool state);

    bool isStereo();
    void setStereo(bool state);

    void squelchAuto();
    bool isSquelchEnabled();
    void setSquelchEnabled(bool state);

    float getSignalLevel();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();

    void setOutputDevice(int device_id);
    int getOutputDevice();

    void setDemodulatorType(int demod_type_in);
    int getDemodulatorType();

    void setBandwidth(int bw);
    int getBandwidth();

    void setFrequency(long long freq);
    long long getFrequency();
private:

    void checkBandwidth();

    std::atomic<std::string *> label; //
    bool terminated; //
    bool demodTerminated; //
    bool audioTerminated; //
    bool preDemodTerminated;
    std::atomic<bool> active;
    std::atomic<bool> squelch;
    std::atomic<bool> stereo;

    long long currentFrequency;
    int currentBandwidth;
    int currentDemodType;
  };
