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
    
    void setDemodulatorLock(bool demod_lock_in);
    int getDemodulatorLock();
    
    void setDemodulatorCons(int demod_cons_in);
    int getDemodulatorCons();

    void setBandwidth(int bw);
    int getBandwidth();

    void setGain(float gain_in);
    float getGain();

    void setFrequency(long long freq);
    long long getFrequency();

    void setAudioSampleRate(int sampleRate);
    int getAudioSampleRate();

    bool isFollow();
    void setFollow(bool follow);

    bool isTracking();
    void setTracking(bool tracking);
private:

    void checkBandwidth();

    std::atomic<std::string *> label; //
    std::atomic_bool terminated; //
    std::atomic_bool demodTerminated; //
    std::atomic_bool audioTerminated; //
    std::atomic_bool preDemodTerminated;
    std::atomic_bool active;
    std::atomic_bool squelch;
    std::atomic_bool stereo;

    std::atomic_llong currentFrequency;
    std::atomic_int currentBandwidth;
    std::atomic_int currentDemodType;
    std::atomic_int currentDemodCons;
    std::atomic_int currentOutputDevice;
    std::atomic_int currentAudioSampleRate;
    std::atomic<float> currentAudioGain;
    std::atomic_bool follow, tracking;
  };
