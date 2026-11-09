// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <queue>
#include <vector>
#include <map>
#include <string>
#include <atomic>
#include <memory>
#include "ThreadBlockingQueue.h"
#include "RtAudio.h"
#include "DemodDefs.h"
#include "AudioMixer.h"

#include "AudioThreadInput.h"

class AudioThreadCommand {
public:
    enum class Type {
        AUDIO_THREAD_CMD_NULL, AUDIO_THREAD_CMD_SET_DEVICE, AUDIO_THREAD_CMD_SET_SAMPLE_RATE
    };

    AudioThreadCommand() :
        cmdType(AudioThreadCommand::Type::AUDIO_THREAD_CMD_NULL), int_value(0) {
    }

    AudioThreadCommand::Type cmdType;
    int int_value;
};

typedef ThreadBlockingQueue<AudioThreadCommand> AudioThreadCommandQueue;

typedef std::shared_ptr<AudioThreadCommandQueue> AudioThreadCommandQueuePtr;

class AudioThread : public IOThread {

public:

    AudioThread();
    ~AudioThread() override;

    static void enumerateDevices(std::vector<RtAudio::DeviceInfo> &devs);

    void setInitOutputDevice(int deviceId, int sampleRate_in = -1);
    int getOutputDevice();

    int getSampleRate();

    void run() override;
    void terminate() override;

    bool isActive();
    void setActive(bool state);

    void setGain(float gain_in);

    static std::map<int, int> deviceSampleRate;

    AudioThreadCommandQueue *getCommandQueue();

    //give access to the this AudioThread lock
    std::recursive_mutex& getMutex();

    static void deviceCleanup();
    static void setDeviceSampleRate(int deviceId, int sampleRate);

    //
    void attachControllerThread(std::thread* controllerThread);

    //fields below, only to be used by other AudioThreads !
    size_t underflowCount;
    //protected by m_mutex
    std::vector<AudioThread *> boundThreads;
    MixerVoice mixer;

private:

    std::atomic_bool active;
    std::atomic_int outputDevice;

    RtAudio dac;
    unsigned int nBufferFrames;
    RtAudio::StreamOptions opts;
    RtAudio::StreamParameters parameters;
    AudioThreadCommandQueue cmdQueue;
    int sampleRate;

    //if != nullptr, it mean AudioThread is a controller thread.
    std::thread* controllerThread;

    //The own m_mutex protecting this AudioThread, in particular boundThreads
    std::recursive_mutex m_mutex;

    void setupDevice(int deviceId);
    void setSampleRate(int sampleRate_in);

    void bindThread(AudioThread *other);
    void removeThread(AudioThread *other);
    void resetAllPrebuffers();

    static std::map<int, AudioThread* > deviceController;

    //The mutex protecting static deviceController, deviceThread and deviceSampleRate access.
    static std::recursive_mutex m_device_mutex;
};
