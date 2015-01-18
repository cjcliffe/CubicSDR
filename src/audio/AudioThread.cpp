#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include <algorithm>
#include "DemodulatorThread.h"

#ifdef USE_MIXER
std::map<int, AudioThread *> AudioThread::deviceController;
std::map<int, std::thread *> AudioThread::deviceThread;
#endif

AudioThread::AudioThread(AudioThreadInputQueue *inputQueue, DemodulatorThreadCommandQueue* threadQueueNotify) :
        currentInput(NULL), inputQueue(inputQueue), audioQueuePtr(0), underflowCount(0), terminated(false), active(false), outputDevice(-1), gain(
                1.0), threadQueueNotify(threadQueueNotify) {
#ifdef USE_MIXER
    boundThreads = new std::vector<AudioThread *>;
#endif
}

AudioThread::~AudioThread() {
#ifdef USE_MIXER
    delete boundThreads.load();
#endif
}

#ifdef USE_MIXER
void AudioThread::bindThread(AudioThread *other) {
    if (std::find(boundThreads.load()->begin(), boundThreads.load()->end(), other) == boundThreads.load()->end()) {
        boundThreads.load()->push_back(other);
    }
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

    if (src->terminated) {
        return 1;
    }

    if (status) {
        std::cout << "Audio buffer underflow.." << (src->underflowCount++) << std::endl;
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
            if (srcmix->terminated) {
                continue;
            }
            srcmix->audioQueuePtr = 0;
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
                if (srcmix->terminated) {
                    continue;
                }
                srcmix->audioQueuePtr = 0;
            }
            continue;
        }

        if (srcmix->currentInput->channels == 1) {
            for (int i = 0; i < nBufferFrames; i++) {
                if (srcmix->audioQueuePtr >= srcmix->currentInput->data.size()) {
                    if (srcmix->currentInput) {
                        srcmix->currentInput->decRefCount();
                        srcmix->currentInput = NULL;
                    }
                    if (srcmix->terminated) {
                        continue;
                    }
                    srcmix->inputQueue->pop(srcmix->currentInput);
                    if (srcmix->terminated) {
                        continue;
                    }
                    srcmix->audioQueuePtr = 0;
                }
                if (srcmix->currentInput && srcmix->currentInput->data.size()) {
                    float v = srcmix->currentInput->data[srcmix->audioQueuePtr] * src->gain * srcmix->gain;
                    out[i * 2] += v;
                    out[i * 2 + 1] += v;
                }
                srcmix->audioQueuePtr++;
            }
        } else {
            for (int i = 0, iMax = srcmix->currentInput->channels * nBufferFrames; i < iMax; i++) {
                if (srcmix->audioQueuePtr >= srcmix->currentInput->data.size()) {
                    if (srcmix->currentInput) {
                        srcmix->currentInput->decRefCount();
                        srcmix->currentInput = NULL;
                    }
                    if (srcmix->terminated) {
                        continue;
                    }
                    srcmix->inputQueue->pop(srcmix->currentInput);
                    if (srcmix->terminated) {
                        continue;
                    }
                    srcmix->audioQueuePtr = 0;
                }
                if (srcmix->currentInput && srcmix->currentInput->data.size()) {
                    out[i] = out[i] + srcmix->currentInput->data[srcmix->audioQueuePtr] * src->gain * srcmix->gain;
                }
                srcmix->audioQueuePtr++;
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
        std::cout << "Audio buffer underflow.." << (src->underflowCount++) << std::endl;
    }

    if (!src->currentInput) {
        src->inputQueue->pop(src->currentInput);
        if (src->terminated) {
            return 1;
        }
        src->audioQueuePtr = 0;
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
            if (src->terminated) {
                return 1;
            }
            src->audioQueuePtr = 0;
        }
        return 0;
    }

    if (src->currentInput->channels == 1) {
        for (int i = 0; i < nBufferFrames; i++) {
            if (src->audioQueuePtr >= src->currentInput->data.size()) {
                if (src->currentInput) {
                    src->currentInput->decRefCount();
                    src->currentInput = NULL;
                }
                if (src->terminated) {
                    return 1;
                }
                src->inputQueue->pop(src->currentInput);
                if (src->terminated) {
                    return 1;
                }
                src->audioQueuePtr = 0;
            }
            if (src->currentInput && src->currentInput->data.size()) {
                out[i * 2] = out[i * 2 + 1] = src->currentInput->data[src->audioQueuePtr] * src->gain;
            }
            src->audioQueuePtr++;
        }
    } else {
        for (int i = 0, iMax = src->currentInput->channels * nBufferFrames; i < iMax; i++) {
            if (src->audioQueuePtr >= src->currentInput->data.size()) {
                if (src->currentInput) {
                    src->currentInput->decRefCount();
                    src->currentInput = NULL;
                }
                if (src->terminated) {
                    return 1;
                }
                src->inputQueue->pop(src->currentInput);
                if (src->terminated) {
                    return 1;
                }
                src->audioQueuePtr = 0;
            }
            if (src->currentInput && src->currentInput->data.size()) {
                out[i] = src->currentInput->data[src->audioQueuePtr] * src->gain;
            }
            src->audioQueuePtr++;
        }
    }
    return 0;
}
#endif

void AudioThread::enumerateDevices(std::vector<RtAudio::DeviceInfo> &devs) {
    RtAudio endac;

    int numDevices = endac.getDeviceCount();

    for (int i = 0; i < numDevices; i++) {
        RtAudio::DeviceInfo info = endac.getDeviceInfo(i);

        devs.push_back(info);

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

void AudioThread::setupDevice(int deviceId) {
    parameters.deviceId = deviceId;
    parameters.nChannels = 2;
    parameters.firstChannel = 0;
    unsigned int sampleRate = AUDIO_FREQUENCY;
    unsigned int bufferFrames = 256;

    RtAudio::StreamOptions opts;
    opts.streamName = "CubicSDR Audio Output";

    try {

#ifdef USE_MIXER
        if (deviceController.find(outputDevice.load()) != deviceController.end()) {
            deviceController[outputDevice.load()]->removeThread(this);
        }

        opts.priority = sched_get_priority_max(SCHED_FIFO);
        //    opts.flags = RTAUDIO_MINIMIZE_LATENCY;
        opts.flags = RTAUDIO_SCHEDULE_REALTIME;

        if (deviceController.find(parameters.deviceId) == deviceController.end()) {
            deviceController[parameters.deviceId] = new AudioThread(NULL, NULL);
            deviceController[parameters.deviceId]->setInitOutputDevice(parameters.deviceId);
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
        if (dac.isStreamOpen()) {
            if (dac.isStreamRunning()) {
                dac.stopStream();
            }
            dac.closeStream();
        }

        dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback, (void *) this, &opts);
        dac.startStream();

#endif
    } catch (RtAudioError& e) {
        e.printMessage();
        return;
    }

    outputDevice = deviceId;
}

int AudioThread::getOutputDevice() {
    if (outputDevice == -1) {
        return dac.getDefaultOutputDevice();
    }
    return outputDevice;
}

void AudioThread::setInitOutputDevice(int deviceId) {
    outputDevice = deviceId;
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

    setupDevice((outputDevice.load() == -1) ? (dac.getDefaultOutputDevice()) : outputDevice.load());

    std::cout << "Audio thread started." << std::endl;

    terminated = false;

    while (!terminated) {
        AudioThreadCommand command;
        cmdQueue.pop(command);

        if (command.cmd == AudioThreadCommand::AUDIO_THREAD_CMD_SET_DEVICE) {
            setupDevice(command.int_value);
        }
    }

#if !USE_MIXER
    AudioThreadInput dummy;
    inputQueue->push(&dummy);
#endif

#ifdef USE_MIXER
    if (deviceController[parameters.deviceId] != this) {
        deviceController[parameters.deviceId]->removeThread(this);
    } else {
        try {
            if (dac.isStreamOpen()) {
                if (dac.isStreamRunning()) {
                    dac.stopStream();
                }
                dac.closeStream();
            }
        } catch (RtAudioError& e) {
            e.printMessage();
        }
    }
#else
    try {
        if (dac.isStreamOpen()) {
            if (dac.isStreamRunning()) {
                dac.stopStream();
            }
            dac.closeStream();
        }
    } catch (RtAudioError& e) {
        e.printMessage();
    }
#endif

    if (threadQueueNotify != NULL) {
        DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_AUDIO_TERMINATED);
        tCmd.context = this;
        threadQueueNotify->push(tCmd);
    }
    std::cout << "Audio thread done." << std::endl;
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
#ifdef USE_MIXER
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

AudioThreadCommandQueue *AudioThread::getCommandQueue() {
    return &cmdQueue;
}

void AudioThread::setGain(float gain_in) {
    if (gain < 0.0) {
        gain = 0.0;
    }
    if (gain > 2.0) {
        gain = 2.0;
    }
    gain = gain_in;
}

float AudioThread::getGain() {
    return gain;
}
