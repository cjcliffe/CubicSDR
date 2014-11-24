#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>

AudioThread::AudioThread(AudioThreadInputQueue *inputQueue) :
		inputQueue(inputQueue), stream(NULL), terminated(false) {

}

AudioThread::~AudioThread() {
	PaError err;
	err = Pa_StopStream(stream);
	err = Pa_CloseStream(stream);
	Pa_Terminate();

	std::cout << std::endl << "Audio Thread Done." << std::endl << std::endl;
}

void AudioThread::threadMain() {
	PaError err;
	err = Pa_Initialize();
	if (err != paNoError) {
		std::cout << "Error starting portaudio :(\n";
		return;
	}

	int preferred_device = -1;

	outputParameters.device =
			(preferred_device != -1) ?
					preferred_device : Pa_GetDefaultOutputDevice();
	if (outputParameters.device == paNoDevice) {
		std::cout << "Error: No default output device.\n";
	}

	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(
			outputParameters.device)->defaultHighOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	stream = NULL;

	err = Pa_OpenStream(&stream, NULL, &outputParameters, AUDIO_FREQUENCY,
			paFramesPerBufferUnspecified, paClipOff, NULL, NULL);

	err = Pa_StartStream(stream);
	if (err != paNoError) {
		std::cout << "Error starting stream: " << Pa_GetErrorText(err)
				<< std::endl;
		std::cout << "\tPortAudio error: " << Pa_GetErrorText(err) << std::endl;
	}

	while (!terminated) {
		AudioThreadInput inp;
		inputQueue->pop(inp);
		if (inp.data.size()) {
		    Pa_WriteStream(stream, &inp.data[0], inp.data.size()/2);
		}
	}
}

void AudioThread::terminate() {
    std::cout << "Terminating audio thread.." << std::endl;
    terminated = true;
    AudioThreadInput endCond;   // push an empty input to bump the queue
    inputQueue->push(endCond);
}
