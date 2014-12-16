#pragma once

#include <queue>
#include <vector>
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
        AUTIO_THREAD_CMD_NULL,
        AUTIO_THREAD_CMD_SET_DEVICE,
    };

    AudioThreadCommand() :
            cmd(AUTIO_THREAD_CMD_NULL), int_value(0) {
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

    void threadMain();
    void terminate();

private:
    RtAudio dac;
    AudioThreadCommandQueue cmdQueue;
    DemodulatorThreadCommandQueue* threadQueueNotify;
};

