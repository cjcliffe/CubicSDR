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
    DEMOD_TYPE_NULL, DEMOD_TYPE_AM, DEMOD_TYPE_FM, DEMOD_TYPE_LSB, DEMOD_TYPE_USB
};

class DemodulatorThreadCommand {
public:
    enum DemodulatorThreadCommandEnum {
        SDR_THREAD_CMD_NULL, SDR_THREAD_CMD_SET_BANDWIDTH, SDR_THREAD_CMD_SET_FREQUENCY
    };

    DemodulatorThreadCommand() :
            cmd(cmd), int_value(SDR_THREAD_CMD_NULL) {

    }

    DemodulatorThreadCommand(DemodulatorThreadCommandEnum cmd) :
            cmd(cmd), int_value(0) {

    }

    DemodulatorThreadCommandEnum cmd;
    int int_value;
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
    unsigned int frequency;
    unsigned int inputRate;
    unsigned int bandwidth; // set equal to disable second stage re-sampling?
    unsigned int audioSampleRate;

    DemodulatorType demodType;

    DemodulatorThreadParameters() :
            frequency(0), inputRate(SRATE), bandwidth(200000), audioSampleRate(AUDIO_FREQUENCY), demodType(DEMOD_TYPE_FM) {

    }

    ~DemodulatorThreadParameters() {

    }
};

typedef ThreadQueue<DemodulatorThreadIQData> DemodulatorThreadInputQueue;
typedef ThreadQueue<AudioThreadInput> DemodulatorThreadOutputQueue;
typedef ThreadQueue<DemodulatorThreadCommand> DemodulatorThreadCommandQueue;

class DemodulatorThread {
public:

    DemodulatorThread(DemodulatorThreadInputQueue* pQueue);
    ~DemodulatorThread();

    void threadMain();

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
        visOutQueue = tQueue;
        visOutQueue->set_max_num_items(1);
    }

    void setCommandQueue(DemodulatorThreadCommandQueue *tQueue) {
        commandQueue = tQueue;
    }

    void setAudioInputQueue(AudioThreadInputQueue *tQueue) {
        audioInputQueue = tQueue;
    }

    DemodulatorThreadParameters &getParams() {
        return params;
    }

    void initialize();

    void terminate();

protected:
    DemodulatorThreadInputQueue* inputQueue;
    DemodulatorThreadOutputQueue* visOutQueue;
    DemodulatorThreadCommandQueue* commandQueue;
    AudioThreadInputQueue *audioInputQueue;

    firfilt_crcf fir_filter;

    msresamp_crcf resampler;
    float resample_ratio;

    msresamp_crcf audio_resampler;
    float audio_resample_ratio;

    DemodulatorThreadParameters params;
    DemodulatorThreadParameters last_params;

    freqdem fdem;
    nco_crcf nco_shift;
    int shift_freq;

    std::atomic<bool> terminated;
    std::atomic<bool> initialized;
};
