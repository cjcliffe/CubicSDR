#include "DemodulatorThread.h"
#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

DemodulatorThread::DemodulatorThread(DemodulatorThreadPostInputQueue* pQueue, DemodulatorThreadCommandQueue* threadQueueNotify) :
        postInputQueue(pQueue), visOutQueue(NULL), terminated(false), audioInputQueue(NULL), threadQueueNotify(threadQueueNotify) {

    float kf = 0.5;         // modulation factor
    fdem = freqdem_create(kf);
//    freqdem_print(fdem);
}
DemodulatorThread::~DemodulatorThread() {
}

#ifdef __APPLE__
void *DemodulatorThread::threadMain() {
#else
void DemodulatorThread::threadMain() {
#endif
#ifdef __APPLE__
    pthread_t tID = pthread_self();	 // ID of this thread
    int priority = sched_get_priority_min( SCHED_RR );
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam( tID, SCHED_RR, &prio );
#endif

    msresamp_crcf audio_resampler = NULL;
    msresamp_crcf resampler = NULL;

    std::cout << "Demodulator thread started.." << std::endl;
    while (!terminated) {
        DemodulatorThreadPostIQData inp;
        postInputQueue->pop(inp);

        int bufSize = inp.data.size();

        if (!bufSize) {
            continue;
        }

        if (resampler == NULL) {
            resampler = inp.resampler;
            audio_resampler = inp.audio_resampler;
        } else if (resampler != inp.resampler) {
            msresamp_crcf_destroy(resampler);
            msresamp_crcf_destroy(audio_resampler);
            resampler = inp.resampler;
            audio_resampler = inp.audio_resampler;
        }

        int out_size = ceil((float) (bufSize) * inp.resample_ratio);
        liquid_float_complex resampled_data[out_size];

        unsigned int num_written;
        msresamp_crcf_execute(resampler, &inp.data[0], bufSize, resampled_data, &num_written);

        float audio_resample_ratio = inp.audio_resample_ratio;

        float demod_output[num_written];

        freqdem_demodulate_block(fdem, resampled_data, num_written, demod_output);

        liquid_float_complex demod_audio_data[num_written];

        for (int i = 0; i < num_written; i++) {
            demod_audio_data[i].real = demod_output[i];
            demod_audio_data[i].imag = 0;
        }

        int audio_out_size = ceil((float) (num_written) * audio_resample_ratio);
        liquid_float_complex resampled_audio_output[audio_out_size];

        unsigned int num_audio_written;
        msresamp_crcf_execute(audio_resampler, demod_audio_data, num_written, resampled_audio_output, &num_audio_written);

        std::vector<float> newBuffer;
        newBuffer.resize(num_audio_written * 2);
        for (int i = 0; i < num_audio_written; i++) {
            liquid_float_complex y = resampled_audio_output[i];

            newBuffer[i * 2] = y.real;
            newBuffer[i * 2 + 1] = y.real;
        }

        AudioThreadInput ati;
        ati.data = newBuffer;

        if (audioInputQueue != NULL) {
            audioInputQueue->push(ati);
        }

        if (visOutQueue != NULL) {
            visOutQueue->push(ati);
        }
    }

    if (resampler != NULL) {
        msresamp_crcf_destroy(resampler);
    }
    if (audio_resampler != NULL) {
        msresamp_crcf_destroy(audio_resampler);
    }

    std::cout << "Demodulator thread done." << std::endl;
    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
}

void DemodulatorThread::terminate() {
    terminated = true;
    DemodulatorThreadPostIQData inp;    // push dummy to nudge queue
    postInputQueue->push(inp);
}
