#pragma once

#include <vector>
#include "wx/defs.h"
#include "wx/string.h"
#include "wx/object.h"
#include "CubicSDRDefs.h"
#include "AudioThread.h"

enum DemodulatorType {
    DEMOD_TYPE_NULL, DEMOD_TYPE_AM, DEMOD_TYPE_FM, DEMOD_TYPE_LSB, DEMOD_TYPE_USB, DEMOD_TYPE_WFM
};

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

class DemodulatorThreadParameters: public wxObject {
public:
    unsigned int inputRate;
    unsigned int inputResampleRate; // set equal to disable second stage re-sampling?
    unsigned int demodResampleRate;
    unsigned int filterFrequency;
    unsigned int audioSampleRate;
    AudioThreadInputQueue *audioInputQueue;

    DemodulatorType demodType;

    DemodulatorThreadParameters() :
        audioInputQueue(NULL), inputRate(SRATE), inputResampleRate(200000), demodResampleRate(100000), audioSampleRate(48000), filterFrequency(32000), demodType(DEMOD_TYPE_WFM) {

    }

    ~DemodulatorThreadParameters() {

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
