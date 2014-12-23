#pragma once

#include "ThreadQueue.h"
#include "CubicSDRDefs.h"
#include "liquid/liquid.h"

#include <atomic>

enum DemodulatorType {
	DEMOD_TYPE_NULL,
	DEMOD_TYPE_AM,
	DEMOD_TYPE_FM,
	DEMOD_TYPE_LSB, DEMOD_TYPE_USB
};

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
            cmd(DEMOD_THREAD_CMD_NULL), int_value(0), context(NULL) {

    }

    DemodulatorThreadCommand(DemodulatorThreadCommandEnum cmd) :
            cmd(cmd), int_value(0), context(NULL) {

	}

	DemodulatorThreadCommandEnum cmd;
	void *context;
	int int_value;
};

class DemodulatorThreadControlCommand {
public:
    enum DemodulatorThreadControlCommandEnum {
        DEMOD_THREAD_CMD_CTL_NULL,
        DEMOD_THREAD_CMD_CTL_SQUELCH_AUTO,
        DEMOD_THREAD_CMD_CTL_SQUELCH_OFF
    };

    DemodulatorThreadControlCommand() :
            cmd(DEMOD_THREAD_CMD_CTL_NULL) {
    }

    DemodulatorThreadControlCommandEnum cmd;
};

class DemodulatorThreadIQData {
public:
	unsigned int frequency;
	unsigned int bandwidth;
	std::vector<signed char> *data;
	std::atomic<int> *refCount;

	DemodulatorThreadIQData() :
			frequency(0), bandwidth(0), data(NULL), refCount(NULL) {

	}

	DemodulatorThreadIQData(const DemodulatorThreadIQData& o) {
	    frequency = o.frequency;
	    bandwidth = o.bandwidth;
	    data = o.data;
	    refCount = o.refCount;
	}

    void setRefCount(std::atomic<int> *rc) {
        refCount = rc;
    }

    void cleanup() {
        if (refCount) {
            refCount->store(refCount->load()-1);
            if (refCount->load() == 0) {
                delete data;
                data = NULL;
                delete refCount;
                refCount = NULL;
            }
        }
    }

	~DemodulatorThreadIQData() {

	}
};

class DemodulatorThreadPostIQData {
public:
	std::vector<liquid_float_complex> *data;
	float audio_resample_ratio;
	msresamp_rrrf audio_resampler;
    float resample_ratio;
    msresamp_crcf resampler;

	DemodulatorThreadPostIQData(): audio_resample_ratio(0), audio_resampler(NULL), resample_ratio(0), resampler(NULL), data(NULL) {

	}

    DemodulatorThreadPostIQData(const DemodulatorThreadPostIQData &o) {
        audio_resample_ratio = o.audio_resample_ratio;
        audio_resampler = o.audio_resampler;
        resample_ratio = o.resample_ratio;
        resampler = o.resampler;
        data = o.data;
    }

	~DemodulatorThreadPostIQData() {

	}
};


class DemodulatorThreadAudioData {
public:
	unsigned int frequency;
	unsigned int sampleRate;
	unsigned char channels;

	std::vector<float> *data;

	DemodulatorThreadAudioData() :
			sampleRate(0), frequency(0), channels(0), data(NULL) {

	}

	DemodulatorThreadAudioData(unsigned int frequency, unsigned int sampleRate,
			std::vector<float> *data) :
			data(data), sampleRate(sampleRate), frequency(frequency), channels(
					1) {

	}

	~DemodulatorThreadAudioData() {

	}
};

typedef ThreadQueue<DemodulatorThreadIQData> DemodulatorThreadInputQueue;
typedef ThreadQueue<DemodulatorThreadPostIQData> DemodulatorThreadPostInputQueue;
typedef ThreadQueue<DemodulatorThreadCommand> DemodulatorThreadCommandQueue;
typedef ThreadQueue<DemodulatorThreadControlCommand> DemodulatorThreadControlCommandQueue;


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
