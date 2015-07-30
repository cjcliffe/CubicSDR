#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include <algorithm>
#include "CubicSDR.h"
#include "DemodulatorThread.h"
#include "DemodulatorInstance.h"
#include <memory.h>

std::map<int, AudioThread *> AudioThread::deviceController;
std::map<int, int> AudioThread::deviceSampleRate;
std::map<int, std::thread *> AudioThread::deviceThread;

AudioThread::AudioThread(AudioThreadInputQueue *inputQueue, DemodulatorThreadCommandQueue* threadQueueNotify) : IOThread(),
        currentInput(NULL), inputQueue(inputQueue), gain(
                1.0), threadQueueNotify(threadQueueNotify), sampleRate(0), nBufferFrames(1024) {

	audioQueuePtr.store(0); 
	underflowCount.store(0);
	active.store(false);
	outputDevice.store(-1);

    boundThreads = new std::vector<AudioThread *>;
}

AudioThread::~AudioThread() {
    delete boundThreads.load();
}

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
    std::map<int, AudioThread *>::iterator i;

    for (i = deviceController.begin(); i != deviceController.end(); i++) {
        i->second->terminate();
    }
}

static int audioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    AudioThread *src = (AudioThread *) userData;
    float *out = (float*) outputBuffer;
    memset(out, 0, nBufferFrames * 2 * sizeof(float));

    if (src->isTerminated()) {
        return 1;
    }

    if (status) {
        std::cout << "Audio buffer underflow.." << (src->underflowCount++) << std::endl;
    }

    if (src->boundThreads.load()->empty()) {
        return 0;
    }

    float peak = 0.0;

    for (int j = 0; j < src->boundThreads.load()->size(); j++) {
        AudioThread *srcmix = (*(src->boundThreads.load()))[j];
        if (srcmix->isTerminated() || !srcmix->inputQueue || srcmix->inputQueue->empty() || !srcmix->isActive()) {
            continue;
        }

        if (!srcmix->currentInput) {
            srcmix->audioQueuePtr = 0;
            if (srcmix->isTerminated() || srcmix->inputQueue->empty()) {
                continue;
            }
            srcmix->inputQueue->pop(srcmix->currentInput);
            if (srcmix->isTerminated()) {
                continue;
            }
            continue;
        }

//        std::lock_guard < std::mutex > lock(srcmix->currentInput->m_mutex);

        if (srcmix->currentInput->sampleRate != src->getSampleRate()) {
            while (srcmix->inputQueue->size()) {
                srcmix->inputQueue->pop(srcmix->currentInput);
                if (srcmix->currentInput) {
                    if (srcmix->currentInput->sampleRate == src->getSampleRate()) {
                        break;
                    }
                    srcmix->currentInput->decRefCount();
                }
                srcmix->currentInput = NULL;
            }

            srcmix->audioQueuePtr = 0;

            if (!srcmix->currentInput) {
                continue;
            }
        }


        if (srcmix->currentInput->channels == 0 || !srcmix->currentInput->data.size()) {
            if (!srcmix->inputQueue->empty()) {
                srcmix->audioQueuePtr = 0;
                if (srcmix->currentInput) {
                    srcmix->currentInput->decRefCount();
                    srcmix->currentInput = NULL;
                }
                if (srcmix->isTerminated() || srcmix->inputQueue->empty()) {
                    continue;
                }
                srcmix->inputQueue->pop(srcmix->currentInput);
                if (srcmix->isTerminated()) {
                    continue;
                }
            }
            continue;
        }

        float mixPeak = srcmix->currentInput->peak * srcmix->gain;

        if (srcmix->currentInput->channels == 1) {
            for (int i = 0; i < nBufferFrames; i++) {
                if (srcmix->audioQueuePtr >= srcmix->currentInput->data.size()) {
                    srcmix->audioQueuePtr = 0;
                    if (srcmix->currentInput) {
                        srcmix->currentInput->decRefCount();
                        srcmix->currentInput = NULL;
                    }
                    if (srcmix->isTerminated() || srcmix->inputQueue->empty()) {
                        break;
                    }
                    srcmix->inputQueue->pop(srcmix->currentInput);
                    if (srcmix->isTerminated()) {
                        break;
                    }
                    float srcPeak = srcmix->currentInput->peak * srcmix->gain;
                    if (mixPeak < srcPeak) {
                        mixPeak = srcPeak;
                    }
                }
                if (srcmix->currentInput && srcmix->currentInput->data.size()) {
                    float v = srcmix->currentInput->data[srcmix->audioQueuePtr] * srcmix->gain;
                    out[i * 2] += v;
                    out[i * 2 + 1] += v;
                }
                srcmix->audioQueuePtr++;
            }
        } else {
            for (int i = 0, iMax = srcmix->currentInput->channels * nBufferFrames; i < iMax; i++) {
                if (srcmix->audioQueuePtr >= srcmix->currentInput->data.size()) {
                    srcmix->audioQueuePtr = 0;
                    if (srcmix->currentInput) {
                        srcmix->currentInput->decRefCount();
                        srcmix->currentInput = NULL;
                    }
                    if (srcmix->isTerminated() || srcmix->inputQueue->empty()) {
                        break;
                    }
                    srcmix->inputQueue->pop(srcmix->currentInput);
                    if (srcmix->isTerminated()) {
                        break;
                    }
                    float srcPeak = srcmix->currentInput->peak * srcmix->gain;
                    if (mixPeak < srcPeak) {
                        mixPeak = srcPeak;
                    }
                }
                if (srcmix->currentInput && srcmix->currentInput->data.size()) {
                    out[i] = out[i] + srcmix->currentInput->data[srcmix->audioQueuePtr] * srcmix->gain;
                }
                srcmix->audioQueuePtr++;
            }
        }

        peak += mixPeak;
    }

    if (peak > 1.0) {
        for (int i = 0; i < nBufferFrames * 2; i++) {
            out[i] /= peak;
        }
    }
    return 0;
}

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

void AudioThread::setDeviceSampleRate(int deviceId, int sampleRate) {
    if (deviceController.find(deviceId) != deviceController.end()) {
        AudioThreadCommand refreshDevice;
        refreshDevice.cmd = AudioThreadCommand::AUDIO_THREAD_CMD_SET_SAMPLE_RATE;
        refreshDevice.int_value = sampleRate;
        deviceController[deviceId]->getCommandQueue()->push(refreshDevice);
    }
}

void AudioThread::setSampleRate(int sampleRate) {
    if (deviceController[outputDevice.load()] == this) {
        deviceSampleRate[outputDevice.load()] = sampleRate;

        dac.stopStream();
        dac.closeStream();

        for (int j = 0; j < boundThreads.load()->size(); j++) {
            AudioThread *srcmix = (*(boundThreads.load()))[j];
            srcmix->setSampleRate(sampleRate);
        }

        std::vector<DemodulatorInstance *>::iterator demod_i;
        std::vector<DemodulatorInstance *> *demodulators;

        demodulators = &wxGetApp().getDemodMgr().getDemodulators();

        for (demod_i = demodulators->begin(); demod_i != demodulators->end(); demod_i++) {
            if ((*demod_i)->getOutputDevice() == outputDevice.load()) {
                (*demod_i)->setAudioSampleRate(sampleRate);
            }
        }

        dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &nBufferFrames, &audioCallback, (void *) this, &opts);
        dac.startStream();
    }

    this->sampleRate = sampleRate;
}

int AudioThread::getSampleRate() {
    return this->sampleRate;
}

void AudioThread::setupDevice(int deviceId) {
    parameters.deviceId = deviceId;
    parameters.nChannels = 2;
    parameters.firstChannel = 0;

    opts.streamName = "CubicSDR Audio Output";

    try {
        if (deviceController.find(outputDevice.load()) != deviceController.end()) {
            deviceController[outputDevice.load()]->removeThread(this);
        }
#ifndef _MSC_VER
        opts.priority = sched_get_priority_max(SCHED_FIFO);
#endif
        //    opts.flags = RTAUDIO_MINIMIZE_LATENCY;
        opts.flags = RTAUDIO_SCHEDULE_REALTIME;

        if (deviceSampleRate.find(parameters.deviceId) != deviceSampleRate.end()) {
            sampleRate = deviceSampleRate[parameters.deviceId];
        } else {
        	std::cout << "Error, device sample rate wasn't initialized?" << std::endl;
        	return;
//            sampleRate = AudioThread::getDefaultAudioSampleRate();
//            deviceSampleRate[parameters.deviceId] = sampleRate;
        }

        if (deviceController.find(parameters.deviceId) == deviceController.end()) {
            deviceController[parameters.deviceId] = new AudioThread(NULL, NULL);

            deviceController[parameters.deviceId]->setInitOutputDevice(parameters.deviceId, sampleRate);
            deviceController[parameters.deviceId]->bindThread(this);

            deviceThread[parameters.deviceId] = new std::thread(&AudioThread::threadMain, deviceController[parameters.deviceId]);
        } else if (deviceController[parameters.deviceId] == this) {
            dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &nBufferFrames, &audioCallback, (void *) this, &opts);
            dac.startStream();
        } else {
            deviceController[parameters.deviceId]->bindThread(this);
        }
        active = true;

    } catch (RtAudioError& e) {
        e.printMessage();
        return;
    }
    if (deviceId != -1) {
        outputDevice = deviceId;
    }
}

int AudioThread::getOutputDevice() {
    if (outputDevice == -1) {
        return dac.getDefaultOutputDevice();
    }
    return outputDevice;
}

void AudioThread::setInitOutputDevice(int deviceId, int sampleRate) {
    outputDevice = deviceId;
    if (sampleRate == -1) {
        if (deviceSampleRate.find(parameters.deviceId) != deviceSampleRate.end()) {
           sampleRate = deviceSampleRate[deviceId];
        }
    } else {
        deviceSampleRate[deviceId] = sampleRate;
    }
    this->sampleRate = sampleRate;
}

void AudioThread::run() {
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

    while (!terminated) {
        AudioThreadCommand command;
        cmdQueue.pop(command);

        if (command.cmd == AudioThreadCommand::AUDIO_THREAD_CMD_SET_DEVICE) {
            setupDevice(command.int_value);
        }
        if (command.cmd == AudioThreadCommand::AUDIO_THREAD_CMD_SET_SAMPLE_RATE) {
            setSampleRate(command.int_value);
        }
    }

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
