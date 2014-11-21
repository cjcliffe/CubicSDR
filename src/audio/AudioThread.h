#pragma once

#include <queue>
#include <vector>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "AudioThreadQueue.h"
#include "portaudio.h"


static int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData);

class AudioThread: public wxThread {
public:
    std::queue<std::vector<float> > audio_queue;
    unsigned int audio_queue_ptr;

    AudioThread(AudioThreadQueue* pQueue, int id = 0);
    ~AudioThread();

protected:
    virtual ExitCode Entry();
    AudioThreadQueue* m_pQueue;
    int m_ID;

    PaStreamParameters outputParameters;
    PaStream *stream;
};
