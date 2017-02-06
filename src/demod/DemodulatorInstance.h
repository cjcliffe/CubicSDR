// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>
#include <map>
#include <thread>

#include "DemodulatorThread.h"
#include "DemodulatorPreThread.h"

#include "ModemDigital.h"
#include "ModemAnalog.h"

#if ENABLE_DIGITAL_LAB
#include "DigitalConsole.h"
#endif

class DemodVisualCue {
public:
    DemodVisualCue();
    ~DemodVisualCue();
    
    void triggerSquelchBreak(int counter);
    int getSquelchBreak();
    
    void step();
private:
    std::atomic_int squelchBreak;
};


class DemodulatorInstance {
public:

#ifdef __APPLE__
    pthread_t t_PreDemod;
    pthread_t t_Demod;
#else
    std::thread *t_PreDemod = nullptr;
    std::thread *t_Demod = nullptr;
#endif

    AudioThread *audioThread = nullptr;
    std::thread *t_Audio = nullptr;

    DemodulatorInstance();
    ~DemodulatorInstance();

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue);

    void run();
    void terminate();
    std::string getLabel();
    void setLabel(std::string labelStr);

    bool isTerminated();
    void updateLabel(long long freq);

    bool isActive();
    void setActive(bool state);

    void squelchAuto();
    bool isSquelchEnabled();
    void setSquelchEnabled(bool state);

    float getSignalLevel();
    float getSignalFloor();
    float getSignalCeil();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();

    void setOutputDevice(int device_id);
    int getOutputDevice();

    void setDemodulatorType(std::string demod_type_in);
    std::string getDemodulatorType();

    std::wstring getDemodulatorUserLabel();
    void setDemodulatorUserLabel(const std::wstring& demod_user_label);
 
    void setDemodulatorLock(bool demod_lock_in);
    int getDemodulatorLock();

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

    bool isDeltaLock();
    void setDeltaLock(bool lock);
    void setDeltaLockOfs(int lockOfs);
    int getDeltaLockOfs();

    bool isMuted();
    void setMuted(bool muted);

    DemodVisualCue *getVisualCue();
    
    DemodulatorThreadInputQueue *getIQInputDataPipe();

    ModemArgInfoList getModemArgs();
    std::string readModemSetting(std::string setting);
    ModemSettings readModemSettings();
    void writeModemSetting(std::string setting, std::string value);
    void writeModemSettings(ModemSettings settings);
    
    bool isModemInitialized();
    std::string getModemType();
    ModemSettings getLastModemSettings(std::string demodType);

#if ENABLE_DIGITAL_LAB
    ModemDigitalOutput *getOutput();
    void showOutput();
    void hideOutput();
    void closeOutput();
#endif
        
protected:
    DemodulatorThreadInputQueue* pipeIQInputData;
    DemodulatorThreadPostInputQueue* pipeIQDemodData;
    AudioThreadInputQueue *pipeAudioData;
    DemodulatorPreThread *demodulatorPreThread;
    DemodulatorThread *demodulatorThread;
    DemodulatorThreadControlCommandQueue *threadQueueControl;

private:

    std::atomic<std::string *> label; //
    // User editable buffer, 16 bit string.
    std::atomic<std::wstring *> user_label; 

    std::atomic_bool active;
    std::atomic_bool squelch;
    std::atomic_bool muted;
    std::atomic_bool deltaLock;
    std::atomic_int deltaLockOfs;

    std::atomic_int currentOutputDevice;
    std::atomic<float> currentAudioGain;
    std::atomic_bool follow, tracking;
    std::map<std::string, ModemSettings> lastModemSettings;
    std::map<std::string, int> lastModemBandwidth;
    DemodVisualCue visualCue;
#if ENABLE_DIGITAL_LAB
    ModemDigitalOutput *activeOutput;
#endif
};
