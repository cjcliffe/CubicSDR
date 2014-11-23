#pragma once

#include <queue>
#include <vector>
#include <string>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "AudioThread.h"
#include "ThreadQueue.h"
#include "portaudio.h"

class AudioThreadInput {
public:
    int frequency;
    int sampleRate;

    std::vector<float> data;
};

typedef ThreadQueue<AudioThreadInput> AudioThreadInputQueue;

class AudioThread {
public:
     AudioThread(AudioThreadInputQueue *inputQueue);

    ~AudioThread();

    void threadMain();

private:
    AudioThreadInputQueue *inputQueue;
    PaStreamParameters outputParameters;
    PaStream *stream;
};

