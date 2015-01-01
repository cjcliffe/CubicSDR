#pragma once

#include <queue>
#include <vector>

#include "DemodDefs.h"
#include "AudioThread.h"

typedef ThreadQueue<AudioThreadInput *> DemodulatorThreadOutputQueue;

#define DEMOD_VIS_SIZE 1024

class DemodulatorThread {
public:

    DemodulatorThread(DemodulatorThreadPostInputQueue* pQueueIn, DemodulatorThreadControlCommandQueue *threadQueueControl,
            DemodulatorThreadCommandQueue* threadQueueNotify);
    ~DemodulatorThread();

#ifdef __APPLE__
    void *threadMain();
#else
    void threadMain();
#endif

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
        visOutQueue = tQueue;
    }

    void setAudioInputQueue(AudioThreadInputQueue *tQueue) {
        audioInputQueue = tQueue;
    }

    void initialize();
    void terminate();

    void setStereo(bool state);bool isStereo();

    float getSignalLevel();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();

    void setDemodulatorType(DemodulatorType demod_type_in);
    DemodulatorType getDemodulatorType();

#ifdef __APPLE__
    static void *pthread_helper(void *context) {
        return ((DemodulatorThread *) context)->threadMain();
    }
#endif

protected:
    std::deque<AudioThreadInput *> buffers;
    std::deque<AudioThreadInput *>::iterator buffers_i;

    std::vector<liquid_float_complex> resampled_data;
    std::vector<liquid_float_complex> agc_data;
    std::vector<float> agc_am_data;
    std::vector<float> demod_output;
    std::vector<float> demod_output_stereo;
    std::vector<float> resampled_audio_output;
    std::vector<float> resampled_audio_output_stereo;

    DemodulatorThreadPostInputQueue* postInputQueue;
    DemodulatorThreadOutputQueue* visOutQueue;
    AudioThreadInputQueue *audioInputQueue;

    freqdem fdem;
    ampmodem ampdem;
    ampmodem ampdem_usb;
    ampmodem ampdem_lsb;

    agc_crcf agc;

    float am_max;
    float am_max_ma;
    float am_max_maa;

    std::atomic<bool> stereo;
    std::atomic<bool> terminated;
    std::atomic<DemodulatorType> demodulatorType;

    DemodulatorThreadCommandQueue* threadQueueNotify;
    DemodulatorThreadControlCommandQueue *threadQueueControl;
    std::atomic<float> squelch_level;
    float squelch_tolerance;
    std::atomic<float> signal_level;bool squelch_enabled;
};
