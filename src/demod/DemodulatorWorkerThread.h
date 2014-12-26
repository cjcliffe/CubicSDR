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

class DemodulatorWorkerThreadResult {
public:
    enum DemodulatorThreadResultEnum {
        DEMOD_WORKER_THREAD_RESULT_NULL, DEMOD_WORKER_THREAD_RESULT_FILTERS
    };

    DemodulatorWorkerThreadResult() :
            cmd(DEMOD_WORKER_THREAD_RESULT_NULL), fir_filter(NULL), resampler(NULL), resample_ratio(0), audio_resampler(NULL), audio_resample_ratio(
                    0), inputRate(0), bandwidth(0), audioSampleRate(0) {

    }

    DemodulatorWorkerThreadResult(DemodulatorThreadResultEnum cmd) :
            cmd(cmd), fir_filter(NULL), resampler(NULL), resample_ratio(0), audio_resampler(NULL), audio_resample_ratio(0), inputRate(0), bandwidth(
                    0), audioSampleRate(0) {

    }

    DemodulatorThreadResultEnum cmd;

    firfilt_crcf fir_filter;
    msresamp_crcf resampler;
    float resample_ratio;
    msresamp_rrrf audio_resampler;
    float audio_resample_ratio;

    unsigned int inputRate;
    unsigned int bandwidth;
    unsigned int audioSampleRate;

};

class DemodulatorWorkerThreadCommand {
public:
    enum DemodulatorThreadCommandEnum {
        DEMOD_WORKER_THREAD_CMD_NULL, DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS
    };

    DemodulatorWorkerThreadCommand() :
            cmd(DEMOD_WORKER_THREAD_CMD_NULL), frequency(0), inputRate(0), bandwidth(0), audioSampleRate(0) {

    }

    DemodulatorWorkerThreadCommand(DemodulatorThreadCommandEnum cmd) :
            cmd(cmd), frequency(0), inputRate(0), bandwidth(0), audioSampleRate(0) {

    }

    DemodulatorThreadCommandEnum cmd;

    unsigned int frequency;
    unsigned int inputRate;
    unsigned int bandwidth;
    unsigned int audioSampleRate;
};

typedef ThreadQueue<DemodulatorWorkerThreadCommand> DemodulatorThreadWorkerCommandQueue;
typedef ThreadQueue<DemodulatorWorkerThreadResult> DemodulatorThreadWorkerResultQueue;

class DemodulatorWorkerThread {
public:

    DemodulatorWorkerThread(DemodulatorThreadWorkerCommandQueue* in, DemodulatorThreadWorkerResultQueue* out);
    ~DemodulatorWorkerThread();

    void threadMain();

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

    std::atomic<bool> terminated;
};
