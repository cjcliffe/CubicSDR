#pragma once

#include "ThreadQueue.h"
#include "CubicSDRDefs.h"
#include "liquid/liquid.h"

#include <atomic>
#include <mutex>

#define DEMOD_TYPE_NULL 0
#define DEMOD_TYPE_FM 1
#define DEMOD_TYPE_AM 2
#define DEMOD_TYPE_LSB 3
#define DEMOD_TYPE_USB 4


class DemodulatorThread;
class DemodulatorThreadCommand {
public:
    enum DemodulatorThreadCommandEnum {
        DEMOD_THREAD_CMD_NULL,
        DEMOD_THREAD_CMD_SET_BANDWIDTH,
        DEMOD_THREAD_CMD_SET_FREQUENCY,
        DEMOD_THREAD_CMD_DEMOD_PREPROCESS_TERMINATED,
        DEMOD_THREAD_CMD_DEMOD_TERMINATED,
        DEMOD_THREAD_CMD_AUDIO_TERMINATED
    };

    DemodulatorThreadCommand() :
            cmd(DEMOD_THREAD_CMD_NULL), context(NULL), llong_value(0) {

    }

    DemodulatorThreadCommand(DemodulatorThreadCommandEnum cmd) :
            cmd(cmd), context(NULL), llong_value(0) {

    }

    DemodulatorThreadCommandEnum cmd;
    void *context;
    long long llong_value;
};

class DemodulatorThreadControlCommand {
public:
    enum DemodulatorThreadControlCommandEnum {
        DEMOD_THREAD_CMD_CTL_NULL, DEMOD_THREAD_CMD_CTL_SQUELCH_ON, DEMOD_THREAD_CMD_CTL_SQUELCH_OFF, DEMOD_THREAD_CMD_CTL_TYPE
    };

    DemodulatorThreadControlCommand() :
            cmd(DEMOD_THREAD_CMD_CTL_NULL), demodType(DEMOD_TYPE_NULL) {
    }

    DemodulatorThreadControlCommandEnum cmd;
    int demodType;
};

class DemodulatorThreadIQData: public ReferenceCounter {
public:
    long long frequency;
    unsigned int bandwidth;
    std::vector<liquid_float_complex> data;

    DemodulatorThreadIQData() :
            frequency(0), bandwidth(0) {

    }

    ~DemodulatorThreadIQData() {

    }
};

class DemodulatorThreadPostIQData: public ReferenceCounter {
public:
    std::vector<liquid_float_complex> data;
    int bandwidth;
    msresamp_rrrf audioResampler;
    msresamp_rrrf stereoResampler;
    double audioResampleRatio;

    DemodulatorThreadPostIQData() :
            bandwidth(0), audioResampler(NULL), stereoResampler(NULL), audioResampleRatio(0) {

    }

    ~DemodulatorThreadPostIQData() {
        std::lock_guard < std::mutex > lock(m_mutex);
    }
};

class DemodulatorThreadAudioData: public ReferenceCounter {
public:
    long long frequency;
    unsigned int sampleRate;
    unsigned char channels;

    std::vector<float> *data;

    DemodulatorThreadAudioData() :
            frequency(0), sampleRate(0), channels(0), data(NULL) {

    }

    DemodulatorThreadAudioData(long long frequency, unsigned int sampleRate, std::vector<float> *data) :
            frequency(frequency), sampleRate(sampleRate), channels(1), data(data) {

    }

    ~DemodulatorThreadAudioData() {

    }
};

typedef ThreadQueue<DemodulatorThreadIQData *> DemodulatorThreadInputQueue;
typedef ThreadQueue<DemodulatorThreadPostIQData *> DemodulatorThreadPostInputQueue;
typedef ThreadQueue<DemodulatorThreadCommand> DemodulatorThreadCommandQueue;
typedef ThreadQueue<DemodulatorThreadControlCommand> DemodulatorThreadControlCommandQueue;

class DemodulatorThreadParameters {
public:
    long long frequency;
    unsigned int inputRate;
    unsigned int bandwidth; // set equal to disable second stage re-sampling?
    unsigned int audioSampleRate;

    int demodType;

    DemodulatorThreadParameters() :
            frequency(0), inputRate(SRATE), bandwidth(200000), audioSampleRate(
            AUDIO_FREQUENCY), demodType(DEMOD_TYPE_FM) {

    }

    ~DemodulatorThreadParameters() {

    }
};
