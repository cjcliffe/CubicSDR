#pragma once

#include <vector>
#include "wx/defs.h"
#include "wx/string.h"
#include "wx/object.h"

class DemodulatorThreadIQData: public wxObject {
public:
    unsigned int frequency;
    unsigned int bandwidth;
    std::vector<signed char> data;

    DemodulatorThreadIQData(unsigned int bandwidth, unsigned int frequency, std::vector<signed char> data) :
            data(data), frequency(frequency), bandwidth(bandwidth) {

    }

    ~DemodulatorThreadIQData() {

    }
};

class DemodulatorThreadAudioData: public wxObject {
public:
    unsigned int frequency;
    unsigned int sampleRate;
    unsigned char channels;

    std::vector<float> data;

    DemodulatorThreadAudioData(unsigned int frequency, unsigned int sampleRate, std::vector<float> data) :
            data(data), sampleRate(sampleRate), frequency(frequency), channels(1) {

    }

    ~DemodulatorThreadAudioData() {

    }
};

class DemodulatorThreadTask {
public:
    enum DEMOD_THREAD_COMMAND {
        DEMOD_THREAD_EXIT = wxID_EXIT,
        DEMOD_THREAD_NULL = wxID_HIGHEST + 100,
        DEMOD_THREAD_STARTED,
        DEMOD_THREAD_PROCESS,
        DEMOD_THREAD_ERROR,
        DEMOD_THREAD_DATA,
        DEMOD_THREAD_AUDIO_DATA
    };

    DemodulatorThreadTask() :
            m_cmd(DEMOD_THREAD_NULL), data(NULL) {
    }
    DemodulatorThreadTask(DEMOD_THREAD_COMMAND cmd) :
            m_cmd(cmd), data(NULL) {
    }

    DEMOD_THREAD_COMMAND m_cmd;

    DemodulatorThreadIQData *data;
};
