#include "DemodulatorThread.h"
#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

DemodulatorThread::DemodulatorThread(DemodulatorThreadPostInputQueue* pQueue, DemodulatorThreadCommandQueue* threadQueueNotify) :
		postInputQueue(pQueue), visOutQueue(NULL), terminated(false), audioInputQueue(NULL), threadQueueNotify(threadQueueNotify) {

	float kf = 0.5;         // modulation factor
	fdem = freqdem_create(kf);
//    freqdem_print(fdem);
}
DemodulatorThread::~DemodulatorThread() {
}

#ifdef __APPLE__
void *DemodulatorThread::threadMain() {
#else
	void DemodulatorThread::threadMain() {
#endif

	std::cout << "Demodulator thread started.." << std::endl;
	while (!terminated) {
		DemodulatorThreadPostIQData inp;
		postInputQueue->pop(inp);

		int out_size = inp.data.size();

		if (!out_size) {
			continue;
		}

		msresamp_crcf audio_resampler = inp.audio_resampler;
		float audio_resample_ratio = inp.audio_resample_ratio;

		float demod_output[out_size];

		freqdem_demodulate_block(fdem, &inp.data[0], out_size, demod_output);

		liquid_float_complex demod_audio_data[out_size];

		for (int i = 0; i < out_size; i++) {
			demod_audio_data[i].real = demod_output[i];
			demod_audio_data[i].imag = 0;
		}

		int audio_out_size = ceil((float) (out_size) * audio_resample_ratio);
		liquid_float_complex resampled_audio_output[audio_out_size];

		unsigned int num_audio_written;
		msresamp_crcf_execute(audio_resampler, demod_audio_data, out_size, resampled_audio_output, &num_audio_written);

		std::vector<float> newBuffer;
		newBuffer.resize(num_audio_written * 2);
		for (int i = 0; i < num_audio_written; i++) {
			liquid_float_complex y = resampled_audio_output[i];

			newBuffer[i * 2] = y.real;
			newBuffer[i * 2 + 1] = y.real;
		}

		AudioThreadInput ati;
		ati.data = newBuffer;

		if (audioInputQueue != NULL) {
			audioInputQueue->push(ati);
		}

		if (visOutQueue != NULL) {
			visOutQueue->push(ati);
		}
	}

	std::cout << "Demodulator thread done." << std::endl;
	DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED);
	tCmd.context = this;
	threadQueueNotify->push(tCmd);
}

void DemodulatorThread::terminate() {
	terminated = true;
	DemodulatorThreadPostIQData inp;    // push dummy to nudge queue
	postInputQueue->push(inp);
}
