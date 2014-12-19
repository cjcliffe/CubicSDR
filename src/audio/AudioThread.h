#pragma once

#include <queue>
#include <vector>
#include <map>
#include <string>
#include <atomic>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "AudioThread.h"
#include "ThreadQueue.h"
#include "RtAudio.h"
#include "DemodDefs.h"

class AudioThreadInput {
public:
    int frequency;
    int sampleRate;

    std::vector<float> data;
};

class AudioThreadCommand {
public:
    enum AudioThreadCommandEnum {
        AUDIO_THREAD_CMD_NULL, AUDIO_THREAD_CMD_SET_DEVICE
    };

    AudioThreadCommand() :
            cmd(AUDIO_THREAD_CMD_NULL), int_value(0) {
    }

    AudioThreadCommandEnum cmd;
    int int_value;
};

typedef ThreadQueue<AudioThreadInput> AudioThreadInputQueue;
typedef ThreadQueue<AudioThreadCommand> AudioThreadCommandQueue;

class AudioThread {
public:

    AudioThreadInput currentInput;
    AudioThreadInputQueue *inputQueue;
    std::atomic<unsigned int> audio_queue_ptr;
    std::atomic<unsigned int> underflow_count;
    std::atomic<bool> terminated;

    AudioThread(AudioThreadInputQueue *inputQueue, DemodulatorThreadCommandQueue* threadQueueNotify);
    ~AudioThread();

    void enumerateDevices();

    void threadMain();
    void terminate();

    bool isActive();
    void setActive(bool state);

#ifdef __APPLE__
    void bindThread(AudioThread *other);
    void removeThread(AudioThread *other);
#endif

private:
    RtAudio dac;
    RtAudio::StreamParameters parameters;
    AudioThreadCommandQueue cmdQueue;
    DemodulatorThreadCommandQueue* threadQueueNotify;

#ifdef __APPLE__
public:
    static std::map<int,AudioThread *> deviceController;
    static std::map<int,std::thread *> deviceThread;
    static void deviceCleanup();
    std::atomic<std::vector<AudioThread *> *> boundThreads;
    float gain;
    std::atomic<bool> active;
#endif
};

