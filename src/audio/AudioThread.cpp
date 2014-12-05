#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>

AudioThread::AudioThread(AudioThreadInputQueue *inputQueue) :
        inputQueue(inputQueue), terminated(false), audio_queue_ptr(0) {

}

AudioThread::~AudioThread() {

}

static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    AudioThread *src = (AudioThread *) userData;
    float *out = (float*) outputBuffer;
    if (status) {
        std::cout << "Audio buffer underflow.." << std::endl;
    }
    if (!src->audio_queue.size()) {
        for (int i = 0; i < nBufferFrames * 2; i++) {
            out[i] = 0;
        }
        return 0;
    }
    std::vector<float> nextBuffer = src->audio_queue.front();
    for (int i = 0; i < nBufferFrames * 2; i++) {
        out[i] = nextBuffer[src->audio_queue_ptr];
        src->audio_queue_ptr++;
        if (src->audio_queue_ptr == nextBuffer.size()) {
            src->audio_queue.pop();
            src->audio_queue_ptr = 0;
            if (!src->audio_queue.size()) {
                for (int j = i; j < nBufferFrames * 2; j++) {
                    std::cout << "Audio buffer underflow.." << std::endl;
                    out[i] = 0;
                }
                return 0;
            }
            nextBuffer = src->audio_queue.front();
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
    unsigned int bufferFrames = 256;

    try {
        dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback, (void *) this);
        dac.startStream();
    } catch (RtAudioError& e) {
        e.printMessage();
        return;
    }

    while (!terminated) {
        AudioThreadInput inp;
        inputQueue->pop(inp);
        if (inp.data.size()) {
            audio_queue.push(inp.data);
        }
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
