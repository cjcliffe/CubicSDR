#pragma once

#include <vector>

#include "wx/defs.h"
#include "wx/string.h"
#include "wx/object.h"

class AudioThreadData: public wxObject {
public:
    unsigned int frequency;
    unsigned int sampleRate;
    unsigned short channels;

    std::vector<float> data;

    AudioThreadData(unsigned int frequency, unsigned int sampleRate, std::vector<float> data) :
            data(data), sampleRate(sampleRate), frequency(frequency) {
        channels = 1;
    }

    ~AudioThreadData() {

    }
};

class AudioThreadTask {
public:
    enum AUDIO_THREAD_COMMAND {
        AUDIO_THREAD_EXIT = wxID_EXIT,
        AUDIO_THREAD_NULL = wxID_HIGHEST + 200,
        AUDIO_THREAD_STARTED,
        AUDIO_THREAD_PROCESS,
        AUDIO_THREAD_ERROR,
        AUDIO_THREAD_DATA
    };

    AudioThreadTask() :
            m_cmd(AUDIO_THREAD_NULL), data(NULL) {
    }
    AudioThreadTask(AUDIO_THREAD_COMMAND cmd) :
            m_cmd(cmd), data(NULL) {
    }

    AudioThreadData *data;

    AUDIO_THREAD_COMMAND m_cmd;
};
