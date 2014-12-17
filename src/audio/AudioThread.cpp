#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include "DemodulatorThread.h"

AudioThread::AudioThread(AudioThreadInputQueue *inputQueue, DemodulatorThreadCommandQueue* threadQueueNotify) :
        inputQueue(inputQueue), terminated(false), audio_queue_ptr(0), underflow_count(0), threadQueueNotify(threadQueueNotify) {

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

    if (src->audio_queue_ptr == src->currentInput.data.size()) {
        if (src->terminated) {
            return 1;
        }
        src->inputQueue->pop(src->currentInput);
        src->audio_queue_ptr = 0;
    }

    for (int i = 0; i < nBufferFrames * 2; i++) {
        out[i] = src->currentInput.data[src->audio_queue_ptr];
        src->audio_queue_ptr++;
        if (src->audio_queue_ptr == src->currentInput.data.size()) {
            if (src->terminated) {
                return 1;
            }
            src->inputQueue->pop(src->currentInput);
            src->audio_queue_ptr = 0;
        }
    }

    return 0;
}

void AudioThread::threadMain() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();	 // ID of this thread
    int priority = sched_get_priority_min( SCHED_RR );
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam( tID, SCHED_RR, &prio );
#endif

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

    RtAudio::StreamOptions opts;
    opts.flags = RTAUDIO_SCHEDULE_REALTIME;
//    | RTAUDIO_MINIMIZE_LATENCY;
//    opts.flags = RTAUDIO_MINIMIZE_LATENCY;
    opts.streamName = "CubicSDR Audio Output";
//    opts.priority = sched_get_priority_max(SCHED_FIFO);

    try {
        dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback, (void *) this, &opts);
        dac.startStream();
    } catch (RtAudioError& e) {
        e.printMessage();
        return;
    }

    while (!terminated) {
        AudioThreadCommand command;
        cmdQueue.pop(command);
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

    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_AUDIO_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
}

void AudioThread::terminate() {
    terminated = true;
    AudioThreadCommand endCond;   // push an empty input to bump the queue
    cmdQueue.push(endCond);
}
