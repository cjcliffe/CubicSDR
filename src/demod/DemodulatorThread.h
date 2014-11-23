#pragma once

#include <queue>
#include <vector>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "liquid/liquid.h"
#include "AudioThread.h"
#include "ThreadQueue.h"
#include "CubicSDRDefs.h"

enum DemodulatorType {
    DEMOD_TYPE_NULL, DEMOD_TYPE_AM, DEMOD_TYPE_FM, DEMOD_TYPE_LSB, DEMOD_TYPE_USB, DEMOD_TYPE_WFM
};

class DemodulatorThreadIQData {
public:
    unsigned int frequency;
    unsigned int bandwidth;
    std::vector<signed char> data;

    DemodulatorThreadIQData() :
            frequency(0), bandwidth(0) {

    }

    DemodulatorThreadIQData(unsigned int bandwidth, unsigned int frequency, std::vector<signed char> data) :
            data(data), frequency(frequency), bandwidth(bandwidth) {

    }

    ~DemodulatorThreadIQData() {

    }
};

class DemodulatorThreadAudioData {
public:
    unsigned int frequency;
    unsigned int sampleRate;
    unsigned char channels;

    std::vector<float> data;

    DemodulatorThreadAudioData() :
            sampleRate(0), frequency(0), channels(0) {

    }

    DemodulatorThreadAudioData(unsigned int frequency, unsigned int sampleRate, std::vector<float> data) :
            data(data), sampleRate(sampleRate), frequency(frequency), channels(1) {

    }

    ~DemodulatorThreadAudioData() {

    }
};

class DemodulatorThreadParameters {
public:
    unsigned int inputRate;
    unsigned int inputResampleRate; // set equal to disable second stage re-sampling?
    unsigned int demodResampleRate;
    unsigned int filterFrequency;
    unsigned int audioSampleRate;
    AudioThreadInputQueue *audioInputQueue;

    DemodulatorType demodType;

    DemodulatorThreadParameters() :
            audioInputQueue(NULL), inputRate(SRATE), inputResampleRate(200000), demodResampleRate(100000), audioSampleRate(48000), filterFrequency(
                    32000), demodType(DEMOD_TYPE_WFM) {

    }

    ~DemodulatorThreadParameters() {

    }
};

typedef ThreadQueue<DemodulatorThreadIQData> DemodulatorThreadInputQueue;
typedef ThreadQueue<AudioThreadInput> DemodulatorThreadOutputQueue;

class DemodulatorThread {
public:

    DemodulatorThread(DemodulatorThreadInputQueue* pQueue, DemodulatorThreadParameters *params);
    ~DemodulatorThread();

    void threadMain();

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
        visOutQueue = tQueue;
        visOutQueue->set_max_num_items(1);
    }

protected:
    DemodulatorThreadInputQueue* m_pQueue;
    DemodulatorThreadOutputQueue* visOutQueue;

    firfilt_crcf fir_filter;
    firfilt_crcf fir_audio_filter;

    msresamp_crcf resampler;
    float resample_ratio;

    msresamp_crcf second_resampler;
    float second_resampler_ratio;

    msresamp_crcf audio_resampler;
    float audio_resample_ratio;

    DemodulatorThreadParameters params;
    freqdem fdem;
};
