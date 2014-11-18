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
#ifdef WIN32
#include "pa_stream.h"
#include "pa_debugprint.h"
#endif

static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData);

// declare a new type of event, to be used by our AudioThread class:
//wxDECLARE_EVENT(wxEVT_COMMAND_AudioThread_COMPLETED, wxThreadEvent);
//wxDECLARE_EVENT(wxEVT_COMMAND_AudioThread_UPDATE, wxThreadEvent);
//wxDECLARE_EVENT(wxEVT_COMMAND_AudioThread_INPUT, wxThreadEvent);

enum {
    EVENT_AUDIO_INPUT = wxID_HIGHEST + 3
};

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
