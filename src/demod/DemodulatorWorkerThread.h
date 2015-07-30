#pragma once

#include <queue>
#include <vector>

#include "liquid/liquid.h"
#include "AudioThread.h"
#include "ThreadQueue.h"
#include "CubicSDRDefs.h"

class DemodulatorWorkerThreadResult {
public:
    enum DemodulatorThreadResultEnum {
        DEMOD_WORKER_THREAD_RESULT_NULL, DEMOD_WORKER_THREAD_RESULT_FILTERS
    };

    DemodulatorWorkerThreadResult() :
            cmd(DEMOD_WORKER_THREAD_RESULT_NULL), iqResampler(NULL), iqResampleRatio(0), audioResampler(NULL), stereoResampler(NULL), audioResamplerRatio(
                    0), firStereoLeft(NULL), firStereoRight(NULL), iirStereoPilot(NULL), sampleRate(0), bandwidth(0), audioSampleRate(0) {

    }

    DemodulatorWorkerThreadResult(DemodulatorThreadResultEnum cmd) :
            DemodulatorWorkerThreadResult() {
        this->cmd = cmd;
    }

    DemodulatorThreadResultEnum cmd;

    msresamp_crcf iqResampler;
    double iqResampleRatio;
    msresamp_rrrf audioResampler;
    msresamp_rrrf stereoResampler;
    double audioResamplerRatio;

    firfilt_rrrf firStereoLeft;
    firfilt_rrrf firStereoRight;
    iirfilt_crcf iirStereoPilot;

    long long sampleRate;
    unsigned int bandwidth;
    unsigned int audioSampleRate;

};

class DemodulatorWorkerThreadCommand {
public:
    enum DemodulatorThreadCommandEnum {
        DEMOD_WORKER_THREAD_CMD_NULL, DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS
    };

    DemodulatorWorkerThreadCommand() :
            cmd(DEMOD_WORKER_THREAD_CMD_NULL), frequency(0), sampleRate(0), bandwidth(0), audioSampleRate(0) {

    }

    DemodulatorWorkerThreadCommand(DemodulatorThreadCommandEnum cmd) :
            cmd(cmd), frequency(0), sampleRate(0), bandwidth(0), audioSampleRate(0) {

    }

    DemodulatorThreadCommandEnum cmd;

    long long frequency;
    long long sampleRate;
    unsigned int bandwidth;
    unsigned int audioSampleRate;
};

typedef ThreadQueue<DemodulatorWorkerThreadCommand> DemodulatorThreadWorkerCommandQueue;
typedef ThreadQueue<DemodulatorWorkerThreadResult> DemodulatorThreadWorkerResultQueue;

class DemodulatorWorkerThread : public IOThread {
public:

    DemodulatorWorkerThread(DemodulatorThreadWorkerCommandQueue* in, DemodulatorThreadWorkerResultQueue* out);
    ~DemodulatorWorkerThread();

    void run();

    void setCommandQueue(DemodulatorThreadWorkerCommandQueue *tQueue) {
        commandQueue = tQueue;
    }

    void setResultQueue(DemodulatorThreadWorkerResultQueue *tQueue) {
        resultQueue = tQueue;
    }

    void terminate();

protected:

    DemodulatorThreadWorkerCommandQueue *commandQueue;
    DemodulatorThreadWorkerResultQueue *resultQueue;
};
