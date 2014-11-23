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

static int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData);

class AudioThreadInput {
public:
    int frequency;
    int sampleRate;

    std::vector<float> data;
};

typedef ThreadQueue<AudioThreadInput> AudioThreadInputQueue;

class AudioThread {
public:
    std::queue<std::vector<float> > audio_queue;
    unsigned int audio_queue_ptr;

    AudioThread(AudioThreadInputQueue *inputQueue) :
            inputQueue(inputQueue), stream(NULL), audio_queue_ptr(0) {

    }

    ~AudioThread() {
        PaError err;
        err = Pa_StopStream(stream);
        err = Pa_CloseStream(stream);
        Pa_Terminate();

        std::cout << std::endl << "Audio Thread Done." << std::endl << std::endl;
    }

    void threadMain();

private:
    AudioThreadInputQueue *inputQueue;
    PaStreamParameters outputParameters;
    PaStream *stream;
};

