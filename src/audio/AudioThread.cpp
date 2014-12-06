#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>

AudioThread::AudioThread(AudioThreadInputQueue *inputQueue) :
        inputQueue(inputQueue), terminated(false), audio_queue_ptr(0), underflow_count(0) {

}

AudioThread::~AudioThread() {

}

static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    AudioThread *src = (AudioThread *) userData;
    float *out = (float*) outputBuffer;
    if (status) {
        std::cout << "Audio buffer underflow.." << (src->underflow_count++) << std::endl;
    }

    std::queue<std::vector<float> > *audio_queue = src->audio_queue.load();

#ifdef __APPLE__	// Crude re-sync
    int wait_for_it = 0;

    if (audio_queue->empty()) {
		while (wait_for_it++ < 50) {	// Can we wait it out?
			std::this_thread::sleep_for(std::chrono::microseconds(10000));
//    		std::this_thread::yield();
			if (!audio_queue->empty()) {
				std::cout << "Buffer recovery.." << std::endl;
				break;
			}
		}
    }
#endif

    if (audio_queue->empty()) {
        for (int i = 0; i < nBufferFrames * 2; i++) {
            out[i] = 0;
        }
        return 0;
    }

    wait_for_it = 0;

    std::vector<float> nextBuffer = audio_queue->front();
    int nextBufferSize = nextBuffer.size();
    for (int i = 0; i < nBufferFrames * 2; i++) {
        out[i] = nextBuffer[src->audio_queue_ptr];
        src->audio_queue_ptr++;
        if (src->audio_queue_ptr == nextBufferSize) {
            audio_queue->pop();
            src->audio_queue_ptr = 0;
            if (audio_queue->empty()) {

#ifdef __APPLE__
            	while (wait_for_it++ < 50) {	// Can we wait it out?
        			std::this_thread::sleep_for(std::chrono::microseconds(10000));
            		if (!audio_queue->empty()) {
        				std::cout << "Buffer recovery.." << std::endl;
            			break;
            		}
            	}
#endif
            	if (audio_queue->empty()) {
					for (int j = i; j < nBufferFrames * 2; j++) {
						std::cout << "Audio buffer underflow mid request.."	<< (src->underflow_count++) << std::endl;
						out[i] = 0;
					}
					return 0;
				}
            }
            nextBuffer = audio_queue->front();
        }
    }
    return 0;
}

void AudioThread::threadMain() {
    std::cout << "Audio thread initializing.." << std::endl;

    if (dac.getDeviceCount() < 1) {
        std::cout << "No audio devices found!" << std::endl;
        return;
    }

    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
    unsigned int sampleRate = AUDIO_FREQUENCY;
    unsigned int bufferFrames = 0;

    RtAudio::StreamOptions opts;
//    opts.flags =  RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_MINIMIZE_LATENCY;
//    opts.flags = RTAUDIO_MINIMIZE_LATENCY;
    opts.streamName = "CubicSDR Audio Output";
    opts.priority = sched_get_priority_max(SCHED_FIFO);

    audio_queue = new std::queue<std::vector<float> >;

    try {
        dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback, (void *) this, &opts);
        dac.startStream();
    } catch (RtAudioError& e) {
        e.printMessage();
        return;
    }

    while (!terminated) {
        AudioThreadInput inp;
		inputQueue->pop(inp);
		if (inp.data.size()) {
			audio_queue.load()->push(inp.data);
		}
//        std::this_thread::yield();
    }

    try {
        // Stop the stream
        dac.stopStream();
    } catch (RtAudioError& e) {
        e.printMessage();
    }

    if (dac.isStreamOpen()) {
        dac.closeStream();
    }

    std::cout << "Audio thread done." << std::endl;
}

void AudioThread::terminate() {
    terminated = true;
    AudioThreadInput endCond;   // push an empty input to bump the queue
    inputQueue->push(endCond);
}
