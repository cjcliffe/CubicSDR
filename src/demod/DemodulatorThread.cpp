#include "DemodulatorThread.h"
#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

DemodulatorThread::DemodulatorThread(DemodulatorThreadPostInputQueue* pQueue, DemodulatorThreadControlCommandQueue *threadQueueControl, DemodulatorThreadCommandQueue* threadQueueNotify) :
        postInputQueue(pQueue), visOutQueue(NULL), audioInputQueue(NULL), agc(NULL), terminated(false), threadQueueNotify(threadQueueNotify), threadQueueControl(threadQueueControl), squelch_level(0), squelch_tolerance(0), squelch_enabled(false) {

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
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO )-1;
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif

    msresamp_rrrf audio_resampler = NULL;
    msresamp_crcf resampler = NULL;

    agc = agc_crcf_create();
    agc_crcf_set_bandwidth(agc, 1e-3f);

    std::cout << "Demodulator thread started.." << std::endl;
    while (!terminated) {
        DemodulatorThreadPostIQData *inp;
        postInputQueue->pop(inp);

        int bufSize = inp->data.size();

        if (!bufSize) {
            delete inp;
            continue;
        }

        if (resampler == NULL) {
            resampler = inp->resampler;
            audio_resampler = inp->audio_resampler;
        } else if (resampler != inp->resampler) {
            msresamp_crcf_destroy(resampler);
            msresamp_rrrf_destroy(audio_resampler);
            resampler = inp->resampler;
            audio_resampler = inp->audio_resampler;
        }

        int out_size = ceil((float) (bufSize) * inp->resample_ratio);
        liquid_float_complex resampled_data[out_size];
        liquid_float_complex agc_data[out_size];

        unsigned int num_written;
        msresamp_crcf_execute(resampler, &(inp->data[0]), bufSize, resampled_data, &num_written);

        agc_crcf_execute_block(agc, resampled_data, num_written, agc_data);

        float audio_resample_ratio = inp->audio_resample_ratio;
        float demod_output[num_written];

        freqdem_demodulate_block(fdem, agc_data, num_written, demod_output);

        int audio_out_size = ceil((float) (num_written) * audio_resample_ratio);
        float resampled_audio_output[audio_out_size];

        unsigned int num_audio_written;
        msresamp_rrrf_execute(audio_resampler, demod_output, num_written, resampled_audio_output, &num_audio_written);

        AudioThreadInput *ati = new AudioThreadInput;
        ati->channels = 1;
        ati->data.assign(resampled_audio_output,resampled_audio_output+num_audio_written);

        if (audioInputQueue != NULL) {
            if (!squelch_enabled || ((agc_crcf_get_signal_level(agc)) >= 0.1)) {
                audioInputQueue->push(ati);
            }
        }

        if (visOutQueue != NULL && visOutQueue->empty()) {
            AudioThreadInput *ati_vis = new AudioThreadInput;
            ati_vis->channels = ati->channels;

            int num_vis = DEMOD_VIS_SIZE;
            if (num_audio_written > num_written) {
                if (num_vis > num_audio_written) {
                    num_vis = num_audio_written;
                }
                ati_vis->data.assign(ati->data.begin(), ati->data.begin()+num_vis);
            } else {
                if (num_vis > num_written) {
                    num_vis = num_written;
                }
                ati_vis->data.assign(demod_output, demod_output + num_vis);
            }

            visOutQueue->push(ati_vis);
//            std::cout << "Signal: " << agc_crcf_get_signal_level(agc) << " -- " << agc_crcf_get_rssi(agc) << "dB " << std::endl;
        }

        if (!threadQueueControl->empty()) {
            while (!threadQueueControl->empty()) {
                DemodulatorThreadControlCommand command;
                threadQueueControl->pop(command);

                switch (command.cmd) {
                case DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_SQUELCH_AUTO:
                    squelch_level = agc_crcf_get_signal_level(agc);
                    squelch_tolerance = agc_crcf_get_signal_level(agc)/2.0;
                    squelch_enabled = true;
                    break;
                case DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_SQUELCH_OFF:
                    squelch_level = 0;
                    squelch_tolerance = 1;
                    squelch_enabled = false;
                    break;
                default:
                    break;
                }
            }
        }

        delete inp;
    }

    if (resampler != NULL) {
        msresamp_crcf_destroy(resampler);
    }
    if (audio_resampler != NULL) {
        msresamp_rrrf_destroy(audio_resampler);
    }

    agc_crcf_destroy(agc);

    std::cout << "Demodulator thread done." << std::endl;
    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
}

void DemodulatorThread::terminate() {
    terminated = true;
    DemodulatorThreadPostIQData *inp = new DemodulatorThreadPostIQData;    // push dummy to nudge queue
    postInputQueue->push(inp);
}
