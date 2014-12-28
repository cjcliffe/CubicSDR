#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include <algorithm>
#include "DemodulatorThread.h"

#ifdef __APPLE__
std::map<int, AudioThread *> AudioThread::deviceController;
std::map<int, std::thread *> AudioThread::deviceThread;
#endif

AudioThread::AudioThread(AudioThreadInputQueue *inputQueue, DemodulatorThreadCommandQueue* threadQueueNotify) :
        currentInput(NULL), inputQueue(inputQueue), audio_queue_ptr(0), underflow_count(0), terminated(false), active(false), gain(1.0), threadQueueNotify(
                threadQueueNotify) {
#ifdef __APPLE__
    boundThreads = new std::vector<AudioThread *>;
#endif
}

AudioThread::~AudioThread() {
#ifdef __APPLE__
    delete boundThreads.load();
#endif
}

#ifdef __APPLE__
void AudioThread::bindThread(AudioThread *other) {
    boundThreads.load()->push_back(other);
}

void AudioThread::removeThread(AudioThread *other) {
    std::vector<AudioThread *>::iterator i;
    i = std::find(boundThreads.load()->begin(), boundThreads.load()->end(), other);
    if (i != boundThreads.load()->end()) {
        boundThreads.load()->erase(i);
    }
}

void AudioThread::deviceCleanup() {
    std::map<int,AudioThread *>::iterator i;

    for (i = deviceController.begin(); i != deviceController.end(); i++) {
        i->second->terminate();
    }
}

static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    AudioThread *src = (AudioThread *) userData;
    float *out = (float*) outputBuffer;
    memset(out, 0, nBufferFrames * 2 * sizeof(float));
    if (status) {
        std::cout << "Audio buffer underflow.." << (src->underflow_count++) << std::endl;
    }

    if (!src->boundThreads.load()->empty()) {
        src->gain = 1.0 / src->boundThreads.load()->size();
    } else {
        return 0;
    }

    for (int j = 0; j < src->boundThreads.load()->size(); j++) {
        AudioThread *srcmix = (*(src->boundThreads.load()))[j];
        if (srcmix->terminated || !srcmix->inputQueue || srcmix->inputQueue->empty() || !srcmix->isActive()) {
            continue;
        }

        if (!srcmix->currentInput) {
            if (srcmix->terminated) {
                continue;
            }
            srcmix->inputQueue->pop(srcmix->currentInput);
            srcmix->audio_queue_ptr = 0;
            continue;
        }

        std::lock_guard < std::mutex > lock(srcmix->currentInput->m_mutex);

        if (srcmix->currentInput->channels == 0 || !srcmix->currentInput->data.size()) {
            if (!srcmix->inputQueue->empty()) {
                if (srcmix->currentInput) {
                    srcmix->currentInput->decRefCount();
                    srcmix->currentInput = NULL;
                }
                if (srcmix->terminated) {
                    continue;
                }
                srcmix->inputQueue->pop(srcmix->currentInput);
                srcmix->audio_queue_ptr = 0;
            }
            continue;
        }

        if (srcmix->currentInput->channels == 1) {
            for (int i = 0; i < nBufferFrames; i++) {
                if (srcmix->audio_queue_ptr >= srcmix->currentInput->data.size()) {
                    if (srcmix->currentInput) {
                        srcmix->currentInput->decRefCount();
                        srcmix->currentInput = NULL;
                    }
                    if (srcmix->terminated) {
                        continue;
                    }
                    srcmix->inputQueue->pop(srcmix->currentInput);
                    srcmix->audio_queue_ptr = 0;
                }
                if (srcmix->currentInput && srcmix->currentInput->data.size()) {
                    float v = srcmix->currentInput->data[srcmix->audio_queue_ptr] * src->gain;
                    out[i * 2] += v;
                    out[i * 2 + 1] += v;
                }
                srcmix->audio_queue_ptr++;
            }
        } else {
            for (int i = 0, iMax = srcmix->currentInput->channels * nBufferFrames; i < iMax; i++) {
                if (srcmix->audio_queue_ptr >= srcmix->currentInput->data.size()) {
                    if (srcmix->currentInput) {
                        srcmix->currentInput->decRefCount();
                        srcmix->currentInput = NULL;
                    }
                    if (srcmix->terminated) {
                        continue;
                    }
                    srcmix->inputQueue->pop(srcmix->currentInput);
                    srcmix->audio_queue_ptr = 0;
                }
                if (srcmix->currentInput && srcmix->currentInput->data.size()) {
                    out[i] = out[i] + srcmix->currentInput->data[srcmix->audio_queue_ptr] * src->gain;
                }
                srcmix->audio_queue_ptr++;
            }
        }

    }

    return 0;
}

#else

static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    AudioThread *src = (AudioThread *) userData;
    float *out = (float*) outputBuffer;
    memset(out, 0, nBufferFrames * 2 * sizeof(float));
    if (status) {
        std::cout << "Audio buffer underflow.." << (src->underflow_count++) << std::endl;
    }

    if (!src->currentInput) {
        src->inputQueue->pop(src->currentInput);
        src->audio_queue_ptr = 0;
        return 0;
    }

    std::lock_guard < std::mutex > lock(src->currentInput->m_mutex);

    if (src->currentInput->channels == 0 || !src->currentInput->data.size()) {
        if (!src->inputQueue->empty()) {
            if (src->currentInput) {
                src->currentInput->decRefCount();
                src->currentInput = NULL;
            }
            if (src->terminated) {
                return 1;
            }
            src->inputQueue->pop(src->currentInput);
            src->audio_queue_ptr = 0;
        }
        return 0;
    }

    if (src->currentInput->channels == 1) {
        for (int i = 0; i < nBufferFrames; i++) {
            if (src->audio_queue_ptr >= src->currentInput->data.size()) {
                if (src->currentInput) {
                    src->currentInput->decRefCount();
                    src->currentInput = NULL;
                }
                if (src->terminated) {
                    return 1;
                }
                src->inputQueue->pop(src->currentInput);
                src->audio_queue_ptr = 0;
            }
            if (src->currentInput && src->currentInput->data.size()) {
                out[i * 2] = out[i * 2 + 1] = src->currentInput->data[src->audio_queue_ptr] * src->gain;
            }
            src->audio_queue_ptr++;
        }
    } else {
        for (int i = 0, iMax = src->currentInput->channels * nBufferFrames; i < iMax; i++) {
            if (src->audio_queue_ptr >= src->currentInput->data.size()) {
                if (src->currentInput) {
                    src->currentInput->decRefCount();
                    src->currentInput = NULL;
                }
                if (src->terminated) {
                    return 1;
                }
                src->inputQueue->pop(src->currentInput);
                src->audio_queue_ptr = 0;
            }
            if (src->currentInput && src->currentInput->data.size()) {
                out[i] = src->currentInput->data[src->audio_queue_ptr] * src->gain;
            }
            src->audio_queue_ptr++;
        }
    }
    return 0;
}
#endif

void AudioThread::enumerateDevices() {
    int numDevices = dac.getDeviceCount();

    for (int i = 0; i < numDevices; i++) {
        RtAudio::DeviceInfo info = dac.getDeviceInfo(i);

        std::cout << std::endl;

        std::cout << "Audio Device #" << i << " " << info.name << std::endl;
        std::cout << "\tDefault Output? " << (info.isDefaultOutput ? "Yes" : "No") << std::endl;
        std::cout << "\tDefault Input? " << (info.isDefaultOutput ? "Yes" : "No") << std::endl;
        std::cout << "\tInput channels: " << info.inputChannels << std::endl;
        std::cout << "\tOutput channels: " << info.outputChannels << std::endl;
        std::cout << "\tDuplex channels: " << info.duplexChannels << std::endl;

        std::cout << "\t" << "Native formats:" << std::endl;
        RtAudioFormat nFormats = info.nativeFormats;
        if (nFormats & RTAUDIO_SINT8) {
            std::cout << "\t\t8-bit signed integer." << std::endl;
        }
        if (nFormats & RTAUDIO_SINT16) {
            std::cout << "\t\t16-bit signed integer." << std::endl;
        }
        if (nFormats & RTAUDIO_SINT24) {
            std::cout << "\t\t24-bit signed integer." << std::endl;
        }
        if (nFormats & RTAUDIO_SINT32) {
            std::cout << "\t\t32-bit signed integer." << std::endl;
        }
        if (nFormats & RTAUDIO_FLOAT32) {
            std::cout << "\t\t32-bit float normalized between plus/minus 1.0." << std::endl;
        }
        if (nFormats & RTAUDIO_FLOAT64) {
            std::cout << "\t\t32-bit float normalized between plus/minus 1.0." << std::endl;
        }

        std::vector<unsigned int>::iterator srate;

        std::cout << "\t" << "Supported sample rates:" << std::endl;

        for (srate = info.sampleRates.begin(); srate != info.sampleRates.end(); srate++) {
            std::cout << "\t\t" << (*srate) << "hz" << std::endl;
        }

        std::cout << std::endl;
    }
}

void AudioThread::threadMain() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();	 // ID of this thread
    int priority = sched_get_priority_max( SCHED_RR) - 1;
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_RR, &prio);
#endif

    std::cout << "Audio thread initializing.." << std::endl;

    if (dac.getDeviceCount() < 1) {
        std::cout << "No audio devices found!" << std::endl;
        return;
    }

    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
    unsigned int sampleRate = AUDIO_FREQUENCY;
    unsigned int bufferFrames = 256;

    RtAudio::StreamOptions opts;
    opts.streamName = "CubicSDR Audio Output";

    try {

#ifdef __APPLE__
        opts.priority = sched_get_priority_max(SCHED_FIFO);
        //    opts.flags = RTAUDIO_MINIMIZE_LATENCY;
        opts.flags = RTAUDIO_SCHEDULE_REALTIME;

        if (deviceController.find(parameters.deviceId) == deviceController.end()) {
            deviceController[parameters.deviceId] = new AudioThread(NULL, NULL);
            deviceController[parameters.deviceId]->bindThread(this);
            deviceThread[parameters.deviceId] = new std::thread(&AudioThread::threadMain, deviceController[parameters.deviceId]);
        } else if (deviceController[parameters.deviceId] == this) {
            dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback, (void *) this, &opts);
            dac.startStream();
        } else {
            deviceController[parameters.deviceId]->bindThread(this);
        }
        active = true;
#else
        dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback, (void *) this, &opts);
        dac.startStream();

#endif
    } catch (RtAudioError& e) {
        e.printMessage();
        return;
    }

    while (!terminated) {
        AudioThreadCommand command;
        cmdQueue.pop(command);
    }

#ifdef __APPLE__
    if (deviceController[parameters.deviceId] != this) {
        deviceController[parameters.deviceId]->removeThread(this);
    } else {
        try {
            dac.stopStream();
            dac.closeStream();
        } catch (RtAudioError& e) {
            e.printMessage();
        }
    }
#else
    try {
        // Stop the stream
        dac.stopStream();
        dac.closeStream();
    } catch (RtAudioError& e) {
        e.printMessage();
    }

    if (dac.isStreamOpen()) {
        dac.closeStream();
    }
#endif

    std::cout << "Audio thread done." << std::endl;

    if (threadQueueNotify != NULL) {
        DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_AUDIO_TERMINATED);
        tCmd.context = this;
        threadQueueNotify->push(tCmd);
    }
}

void AudioThread::terminate() {
    terminated = true;
    AudioThreadCommand endCond;   // push an empty input to bump the queue
    cmdQueue.push(endCond);
}

bool AudioThread::isActive() {
    return active;
}

void AudioThread::setActive(bool state) {
#ifdef __APPLE__
    AudioThreadInput *dummy;
    if (state && !active) {
        while (!inputQueue->empty()) {  // flush queue
            inputQueue->pop(dummy);
            if (dummy) {
                dummy->decRefCount();
            }
        }
        deviceController[parameters.deviceId]->bindThread(this);
    } else if (!state && active) {
        deviceController[parameters.deviceId]->removeThread(this);
        while (!inputQueue->empty()) {  // flush queue
            inputQueue->pop(dummy);
            if (dummy) {
                dummy->decRefCount();
            }
        }
    }
#endif
    active = state;
}
