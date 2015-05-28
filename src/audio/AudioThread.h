#pragma once

#include <queue>
#include <vector>
#include <map>
#include <string>
#include <atomic>

#include "AudioThread.h"
#include "ThreadQueue.h"
#include "RtAudio.h"
#include "DemodDefs.h"

class AudioThreadInput: public ReferenceCounter {
public:
    long long frequency;
    int sampleRate;
    int channels;
    float peak;
    std::vector<float> data;
    std::mutex busy_update;

    AudioThreadInput() :
            frequency(0), sampleRate(0), channels(0), peak(0) {

    }

    ~AudioThreadInput() {
        std::lock_guard < std::mutex > lock(m_mutex);
    }
};

class AudioThreadCommand {
public:
    enum AudioThreadCommandEnum {
        AUDIO_THREAD_CMD_NULL, AUDIO_THREAD_CMD_SET_DEVICE, AUDIO_THREAD_CMD_SET_SAMPLE_RATE
    };

    AudioThreadCommand() :
            cmd(AUDIO_THREAD_CMD_NULL), int_value(0) {
    }

    AudioThreadCommandEnum cmd;
    int int_value;
};

typedef ThreadQueue<AudioThreadInput *> AudioThreadInputQueue;
typedef ThreadQueue<AudioThreadCommand> AudioThreadCommandQueue;

class AudioThread {
public:

    AudioThreadInput *currentInput;
    AudioThreadInputQueue *inputQueue;
    std::atomic<unsigned int> audioQueuePtr;
    std::atomic<unsigned int> underflowCount;
    std::atomic<bool> terminated;
    std::atomic<bool> initialized;
    std::atomic<bool> active;
    std::atomic<int> outputDevice;
    std::atomic<float> gain;

    AudioThread(AudioThreadInputQueue *inputQueue, DemodulatorThreadCommandQueue* threadQueueNotify);
    ~AudioThread();

    static void enumerateDevices(std::vector<RtAudio::DeviceInfo> &devs);

    void setupDevice(int deviceId);
    void setInitOutputDevice(int deviceId, int sampleRate=-1);
    int getOutputDevice();
    void setSampleRate(int sampleRate);
    int getSampleRate();
    void threadMain();
    void terminate();

    bool isActive();
    void setActive(bool state);

    void setGain(float gain_in);
    float getGain();

    AudioThreadCommandQueue *getCommandQueue();

private:
    RtAudio dac;
    unsigned int nBufferFrames;
    RtAudio::StreamOptions opts;
    RtAudio::StreamParameters parameters;
    AudioThreadCommandQueue cmdQueue;
    DemodulatorThreadCommandQueue* threadQueueNotify;
    int sampleRate;

public:
    void bindThread(AudioThread *other);
    void removeThread(AudioThread *other);

    static std::map<int,AudioThread *> deviceController;
    static std::map<int,int> deviceSampleRate;
    static std::map<int,std::thread *> deviceThread;
    static void deviceCleanup();
    static void setDeviceSampleRate(int deviceId, int sampleRate);
    std::atomic<std::vector<AudioThread *> *> boundThreads;
};

