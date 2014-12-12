#pragma once

#include <queue>
#include <vector>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "liquid/liquid.h"
#include "CubicSDRDefs.h"
#include "DemodulatorWorkerThread.h"
#include "DemodDefs.h"

class DemodulatorThreadParameters {
public:
    unsigned int frequency;
    unsigned int inputRate;
    unsigned int bandwidth; // set equal to disable second stage re-sampling?
    unsigned int audioSampleRate;

    DemodulatorType demodType;

    DemodulatorThreadParameters() :
            frequency(0), inputRate(SRATE), bandwidth(200000), audioSampleRate(
                    AUDIO_FREQUENCY), demodType(DEMOD_TYPE_FM) {

    }

    ~DemodulatorThreadParameters() {

    }
};

typedef ThreadQueue<AudioThreadInput> DemodulatorThreadOutputQueue;

class DemodulatorThread {
public:

	DemodulatorThread(DemodulatorThreadInputQueue* pQueue, DemodulatorThreadCommandQueue* threadQueueNotify);
	~DemodulatorThread();

#ifdef __APPLE__
	void *threadMain();
#else
	void threadMain();
#endif

	void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
		visOutQueue = tQueue;
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

#ifdef __APPLE__
	static void *pthread_helper(void *context) {
		return ((DemodulatorThread *) context)->threadMain();
	}
#endif

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

	DemodulatorWorkerThread *workerThread;
	std::thread *t_Worker;

	DemodulatorThreadWorkerCommandQueue *workerQueue;
	DemodulatorThreadWorkerResultQueue *workerResults;
	DemodulatorThreadCommandQueue* threadQueueNotify;
};
