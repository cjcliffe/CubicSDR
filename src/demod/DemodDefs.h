#pragma once

#include "ThreadQueue.h"

enum DemodulatorType {
	DEMOD_TYPE_NULL,
	DEMOD_TYPE_AM,
	DEMOD_TYPE_FM,
	DEMOD_TYPE_LSB,
	DEMOD_TYPE_USB
};

class DemodulatorThread;
class DemodulatorThreadCommand {
public:
	enum DemodulatorThreadCommandEnum {
		DEMOD_THREAD_CMD_NULL,
		DEMOD_THREAD_CMD_SET_BANDWIDTH,
		DEMOD_THREAD_CMD_SET_FREQUENCY,
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

class DemodulatorThreadIQData {
public:
	unsigned int frequency;
	unsigned int bandwidth;
	std::vector<signed char> data;

	DemodulatorThreadIQData() :
			frequency(0), bandwidth(0) {

	}

	DemodulatorThreadIQData(unsigned int bandwidth, unsigned int frequency,
			std::vector<signed char> data) :
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

	DemodulatorThreadAudioData(unsigned int frequency, unsigned int sampleRate,
			std::vector<float> data) :
			data(data), sampleRate(sampleRate), frequency(frequency), channels(
					1) {

	}

	~DemodulatorThreadAudioData() {

	}
};

typedef ThreadQueue<DemodulatorThreadIQData> DemodulatorThreadInputQueue;
typedef ThreadQueue<DemodulatorThreadCommand> DemodulatorThreadCommandQueue;
