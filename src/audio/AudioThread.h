#pragma once

#include <queue>
#include <vector>
#include <string>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "AudioThreadQueue.h"
#include "ThreadQueue.h"
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

class AudioThreadNew {
private:
    ThreadQueue<std::string> *threadQueue;
public:
    AudioThreadNew(ThreadQueue<std::string> *tq_in) {
        threadQueue = tq_in;
    }

    void threadMain() {
        while (1) {
            while (!threadQueue->empty()) {
                std::string str;
                if (threadQueue->try_pop(str)) {
                    std::cout << str << std::endl;
                }
            }
        }
    }
};
