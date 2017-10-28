// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>
#include <algorithm>
#include "CubicSDR.h"
#include "DemodulatorThread.h"
#include "DemodulatorInstance.h"
#include <memory.h>
#include <mutex>

//50 ms
#define HEARTBEAT_CHECK_PERIOD_MICROS (50 * 1000) 

std::map<int, AudioThread *> AudioThread::deviceController;
std::map<int, int> AudioThread::deviceSampleRate;
std::map<int, std::thread *> AudioThread::deviceThread;

std::recursive_mutex AudioThread::m_device_mutex;

AudioThread::AudioThread() : IOThread(),
        currentInput(nullptr), inputQueue(nullptr), nBufferFrames(1024), sampleRate(0) {

	audioQueuePtr = 0; 
	underflowCount = 0;
	active.store(false);
	outputDevice.store(-1);
    gain = 1.0;
}

AudioThread::~AudioThread() {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
}

std::recursive_mutex & AudioThread::getMutex()
{
    return m_mutex;
}

void AudioThread::bindThread(AudioThread *other) {

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (std::find(boundThreads.begin(), boundThreads.end(), other) == boundThreads.end()) {
        boundThreads.push_back(other);
    }
}

void AudioThread::removeThread(AudioThread *other) {
    
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto i = std::find(boundThreads.begin(), boundThreads.end(), other);

    if (i != boundThreads.end()) {
        boundThreads.erase(i);
    }
}

void AudioThread::deviceCleanup() {

	std::lock_guard<std::recursive_mutex> lock(m_device_mutex);

    for (auto i = deviceController.begin(); i != deviceController.end(); i++) {
        i->second->terminate();
    }
}

static int audioCallback(void *outputBuffer, void * /* inputBuffer */, unsigned int nBufferFrames, double /* streamTime */, RtAudioStreamStatus status,
        void *userData) {

    float *out = (float*)outputBuffer;

    //Zero output buffer in all cases: this allow to mute audio if no AudioThread data is 
    //actually active.
    ::memset(out, 0, nBufferFrames * 2 * sizeof(float));

    AudioThread *src = (AudioThread *) userData;
   
    std::lock_guard<std::recursive_mutex> lock(src->getMutex());

    if (src->isTerminated()) {
        return 1;
    }

    if (status) {
       std::cout << "Audio buffer underflow.." << (src->underflowCount++) << std::endl << std::flush;
    }

    if (src->boundThreads.empty()) {
       return 0;
    }

    
    double peak = 0.0;
   
    //for all boundThreads
    for (size_t j = 0; j < src->boundThreads.size(); j++) {

        AudioThread *srcmix = src->boundThreads[j];

        //lock every single boundThread srcmix in succession the time we process 
        //its audio samples.
        std::lock_guard<std::recursive_mutex> lock(srcmix->getMutex());

        if (srcmix->isTerminated() || !srcmix->inputQueue || srcmix->inputQueue->empty() || !srcmix->isActive()) {
            continue;
        }

        if (!srcmix->currentInput) {
            srcmix->audioQueuePtr = 0;
           
            if (!srcmix->inputQueue->try_pop(srcmix->currentInput)) {
                continue;
            }
            
            continue;
        }

        if (srcmix->currentInput->sampleRate != src->getSampleRate()) {

            while (srcmix->inputQueue->try_pop(srcmix->currentInput)) {
                
                if (srcmix->currentInput) {
                    if (srcmix->currentInput->sampleRate == src->getSampleRate()) {
                        break;
                    }
                   
                }
                srcmix->currentInput = nullptr;
            } //end while

            srcmix->audioQueuePtr = 0;

            if (!srcmix->currentInput) {
                continue;
            }
        }


        if (srcmix->currentInput->channels == 0 || !srcmix->currentInput->data.size()) {
            if (!srcmix->inputQueue->empty()) {
                srcmix->audioQueuePtr = 0;
                if (srcmix->currentInput) {
                   
                    srcmix->currentInput = nullptr;
                }

                if (!srcmix->inputQueue->try_pop(srcmix->currentInput)) {
                    continue;
                }          
            }
            continue;
        }

        double mixPeak = srcmix->currentInput->peak * srcmix->gain;

        if (srcmix->currentInput->channels == 1) {

            for (unsigned int i = 0; i < nBufferFrames; i++) {

                if (srcmix->audioQueuePtr >= srcmix->currentInput->data.size()) {
                    srcmix->audioQueuePtr = 0;
                    if (srcmix->currentInput) {
                       
                        srcmix->currentInput = nullptr;
                    }

                    if (!srcmix->inputQueue->try_pop(srcmix->currentInput)) {
                        break;
                    }

            
                    double srcPeak = srcmix->currentInput->peak * srcmix->gain;
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
                       
                        srcmix->currentInput = nullptr;
                    }

                    if (!srcmix->inputQueue->try_pop(srcmix->currentInput)) {
                        break;
                    }

                    double srcPeak = srcmix->currentInput->peak * srcmix->gain;
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

    //normalize volume
    if (peak > 1.0) {
        float invPeak = (float)(1.0 / peak);

        for (unsigned int i = 0; i < nBufferFrames * 2; i++) {
            out[i] *= invPeak;
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
        std::cout << "\tDefault Input? " << (info.isDefaultInput ? "Yes" : "No") << std::endl;
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
            std::cout << "\t\t64-bit float normalized between plus/minus 1.0." << std::endl;
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
   
	AudioThread* matchingAudioThread = nullptr;

	//scope lock here to minimize the common unique static lock contention
	{
		std::lock_guard<std::recursive_mutex> lock(m_device_mutex);

		if (deviceController.find(deviceId) != deviceController.end()) {

			matchingAudioThread = deviceController[deviceId];
		}
	}

	//out-of-lock test
	if (matchingAudioThread != nullptr) {

		AudioThreadCommand refreshDevice;
		refreshDevice.cmd = AudioThreadCommand::AUDIO_THREAD_CMD_SET_SAMPLE_RATE;
		refreshDevice.int_value = sampleRate;
		//VSO : blocking push !
		matchingAudioThread->getCommandQueue()->push(refreshDevice);
	}
}

void AudioThread::setSampleRate(int sampleRate) {

	bool outputIsThis = false;

	//scope lock here to minimize the common unique static lock contention
	{
		std::lock_guard<std::recursive_mutex> lock(m_device_mutex);

		if (deviceController[outputDevice.load()] == this) {
			outputIsThis = true;
			deviceSampleRate[outputDevice.load()] = sampleRate;
		}
	}

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (outputIsThis) {
       
        dac.stopStream();
        dac.closeStream();

        for (size_t j = 0; j < boundThreads.size(); j++) {
            AudioThread *srcmix = boundThreads[j];
            srcmix->setSampleRate(sampleRate);
        }

        //make a local copy, snapshot of the list of demodulators
        std::vector<DemodulatorInstancePtr> demodulators = wxGetApp().getDemodMgr().getDemodulators();

        for (auto demod : demodulators) {
            if (demod->getOutputDevice() == outputDevice.load()) {
                demod->setAudioSampleRate(sampleRate);
            }
        }

        dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32, sampleRate, &nBufferFrames, &audioCallback, (void *) this, &opts);
        dac.startStream();
    }

    this->sampleRate = sampleRate;
}

int AudioThread::getSampleRate() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return this->sampleRate;
}

void AudioThread::setupDevice(int deviceId) {

	//global lock to setup the device...
	std::lock_guard<std::recursive_mutex> lock(m_device_mutex);

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
            deviceController[parameters.deviceId] = new AudioThread();

            deviceController[parameters.deviceId]->setInitOutputDevice(parameters.deviceId, sampleRate);
            deviceController[parameters.deviceId]->bindThread(this);

            deviceThread[parameters.deviceId] = new std::thread(&AudioThread::threadMain, deviceController[parameters.deviceId]);
        } else if (deviceController[parameters.deviceId] == this) {
            //Attach callback
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

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (outputDevice == -1) {
        return dac.getDefaultOutputDevice();
    }
    return outputDevice;
}

void AudioThread::setInitOutputDevice(int deviceId, int sampleRate) {
    
	//global lock 
    std::lock_guard<std::recursive_mutex> lock(m_device_mutex);

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

//    std::cout << "Audio thread initializing.." << std::endl;

    if (dac.getDeviceCount() < 1) {
        std::cout << "No audio devices found!" << std::endl;
        return;
    }

    setupDevice((outputDevice.load() == -1) ? (dac.getDefaultOutputDevice()) : outputDevice.load());

//    std::cout << "Audio thread started." << std::endl;
    
    inputQueue = std::static_pointer_cast<AudioThreadInputQueue>(getInputQueue("AudioDataInput"));
    
    //Infinite loop, witing for commands or for termination
    while (!stopping) {
        AudioThreadCommand command;

        if (!cmdQueue.pop(command, HEARTBEAT_CHECK_PERIOD_MICROS)) {
            continue;
        }

        if (command.cmd == AudioThreadCommand::AUDIO_THREAD_CMD_SET_DEVICE) {
            setupDevice(command.int_value);
        }
        if (command.cmd == AudioThreadCommand::AUDIO_THREAD_CMD_SET_SAMPLE_RATE) {
            setSampleRate(command.int_value);
        }
    }
   
    // Drain any remaining inputs, with a non-blocking pop
    if (inputQueue != nullptr) {
        inputQueue->flush();
    }

	//Thread termination, prevent fancy things to happen, lock the whole thing:
	//This way audioThreadCallback is rightly protected from thread termination
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	//Nullify currentInput...
	currentInput = nullptr;

    //Stop : this affects the device list , so must be protected globally.
	std::lock_guard<std::recursive_mutex> global_lock(m_device_mutex);

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

//    std::cout << "Audio thread done." << std::endl;
}

void AudioThread::terminate() {
    IOThread::terminate();
}

bool AudioThread::isActive() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    return active;
}

void AudioThread::setActive(bool state) {

	AudioThread* matchingAudioThread = nullptr;

	//scope lock here to minimize the common unique static lock contention
	{
		std::lock_guard<std::recursive_mutex> lock(m_device_mutex);

		if (deviceController.find(parameters.deviceId) != deviceController.end()) {

			matchingAudioThread = deviceController[parameters.deviceId];
		}
	}

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (matchingAudioThread == nullptr) {
        return;
    }

    if (state && !active && inputQueue) {
		matchingAudioThread->bindThread(this);
    } else if (!state && active) {
		matchingAudioThread->removeThread(this);
    }

    // Activity state changing, clear any inputs
    if(inputQueue) {
        inputQueue->flush();
    }
    active = state;
}

AudioThreadCommandQueue *AudioThread::getCommandQueue() {
    return &cmdQueue;
}

void AudioThread::setGain(float gain_in) {
   
    if (gain_in < 0.0) {
        gain_in = 0.0;
    }
    if (gain_in > 2.0) {
        gain_in = 2.0;
    }
    gain = gain_in;
}
