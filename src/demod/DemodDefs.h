#pragma once

#include "ThreadQueue.h"
#include "CubicSDRDefs.h"
#include "liquid/liquid.h"

#include <atomic>
#include <mutex>

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
            cmd(DEMOD_THREAD_CMD_NULL), context(NULL), int_value(0) {

    }

    DemodulatorThreadCommand(DemodulatorThreadCommandEnum cmd) :
            cmd(cmd), context(NULL), int_value(0) {

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
	std::vector<signed char> data;
    mutable std::mutex m_mutex;

	DemodulatorThreadIQData() :
			frequency(0), bandwidth(0), refCount(0) {

	}

    void setRefCount(int rc) {
        refCount.store(rc);
    }

    void decRefCount() {
        refCount.store(refCount.load()-1);
    }

    int getRefCount() {
        return refCount.load();
    }

	~DemodulatorThreadIQData() {

	}
private:
    std::atomic<int> refCount;

};

class DemodulatorThreadPostIQData {
public:
	std::vector<liquid_float_complex> data;
	float audio_resample_ratio;
	msresamp_rrrf audio_resampler;
    float resample_ratio;
    msresamp_crcf resampler;

	DemodulatorThreadPostIQData(): audio_resample_ratio(0), audio_resampler(NULL), resample_ratio(0), resampler(NULL) {

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
			frequency(0), sampleRate(0), channels(0), data(NULL) {

	}

	DemodulatorThreadAudioData(unsigned int frequency, unsigned int sampleRate,
			std::vector<float> *data) :
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
