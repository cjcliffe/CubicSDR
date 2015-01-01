#include "DemodulatorThread.h"
#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

DemodulatorThread::DemodulatorThread(DemodulatorThreadPostInputQueue* pQueue, DemodulatorThreadControlCommandQueue *threadQueueControl,
        DemodulatorThreadCommandQueue* threadQueueNotify) :
        postInputQueue(pQueue), visOutQueue(NULL), audioInputQueue(NULL), agc(NULL), am_max(1), am_max_ma(1), am_max_maa(1), stereo(false), terminated(
                false), demodulatorType(DemodulatorType::DEMOD_TYPE_FM), threadQueueNotify(threadQueueNotify), threadQueueControl(threadQueueControl), squelch_level(
                0), squelch_tolerance(0), signal_level(0), squelch_enabled(false) {

    fdem = freqdem_create(0.5);
    liquid_ampmodem_type type_lsb = LIQUID_AMPMODEM_LSB;
    ampdem_lsb = ampmodem_create(1.0, 0.5, type_lsb, 1);
    liquid_ampmodem_type type_usb = LIQUID_AMPMODEM_USB;
    ampdem_usb = ampmodem_create(1.0, -0.5, type_usb, 1);
    liquid_ampmodem_type type_dsb = LIQUID_AMPMODEM_DSB;
    ampdem = ampmodem_create(0.5, 0.0, type_dsb, 0);

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
    msresamp_rrrf stereo_resampler = NULL;
    firfilt_rrrf fir_filter = NULL;
    firfilt_rrrf fir_filter2 = NULL;
    msresamp_crcf resampler = NULL;

    double fc = 0.5 * ((double) 36000 / (double) AUDIO_FREQUENCY);         // filter cutoff frequency
    if (fc <= 0) {
        fc = 0;
    }
    if (fc >= 0.5) {
        fc = 0.5;
    }
    float ft = 0.05f;         // filter transition
    float As = 60.0f;         // stop-band attenuation [dB]
    float mu = 0.0f;         // fractional timing offset
    // estimate required filter length and generate filter
    unsigned int h_len = estimate_req_filter_len(ft, As);
    float h[h_len];
    liquid_firdes_kaiser(h_len, fc, As, mu, h);
    fir_filter = firfilt_rrrf_create(h, h_len);
    fir_filter2 = firfilt_rrrf_create(h, h_len);

    unsigned int m = 5;           // filter semi-length
    float slsl = 60.0f;           // filter sidelobe suppression level
    liquid_float_complex x, y;

    firhilbf firR2C = firhilbf_create(m, slsl);
    firhilbf firC2R = firhilbf_create(m, slsl);

    nco_crcf nco_shift = nco_crcf_create(LIQUID_NCO);
    double shift_freq = 0;

    agc = agc_crcf_create();
    agc_crcf_set_bandwidth(agc, 0.9);

    std::cout << "Demodulator thread started.." << std::endl;

    double freq_index = 0;

    while (!terminated) {
        DemodulatorThreadPostIQData *inp;
        postInputQueue->pop(inp);
        std::lock_guard < std::mutex > lock(inp->m_mutex);

        int bufSize = inp->data.size();

        if (!bufSize) {
            inp->decRefCount();
            continue;
        }

        if (resampler == NULL) {
            resampler = inp->resampler;
            audio_resampler = inp->audio_resampler;
            stereo_resampler = inp->stereo_resampler;
        } else if (resampler != inp->resampler) {
            msresamp_crcf_destroy(resampler);
            msresamp_rrrf_destroy(audio_resampler);
            msresamp_rrrf_destroy(stereo_resampler);
            resampler = inp->resampler;
            audio_resampler = inp->audio_resampler;
            stereo_resampler = inp->stereo_resampler;

            ampmodem_reset(ampdem_lsb);
            ampmodem_reset(ampdem_usb);
            ampmodem_reset(ampdem);
            freqdem_reset(fdem);
        }

        int out_size = ceil((double) (bufSize) * inp->resample_ratio);

        if (agc_data.size() != out_size) {
            if (agc_data.capacity() < out_size) {
                agc_data.reserve(out_size);
                agc_am_data.reserve(out_size);
                resampled_data.reserve(out_size);
            }
            agc_data.resize(out_size);
            resampled_data.resize(out_size);
            agc_am_data.resize(out_size);
        }

        unsigned int num_written;
        msresamp_crcf_execute(resampler, &(inp->data[0]), bufSize, &resampled_data[0], &num_written);

        double audio_resample_ratio = inp->audio_resample_ratio;

        if (demod_output.size() != num_written) {
            if (demod_output.capacity() < num_written) {
                demod_output.reserve(num_written);
            }
            demod_output.resize(num_written);
        }

        int audio_out_size = ceil((double) (num_written) * audio_resample_ratio);

        agc_crcf_execute_block(agc, &resampled_data[0], num_written, &agc_data[0]);

        float current_level = 0;

        current_level = ((60.0 / fabs(agc_crcf_get_rssi(agc))) / 15.0 - signal_level);

        if (agc_crcf_get_signal_level(agc) > current_level) {
            current_level = agc_crcf_get_signal_level(agc);
        }

        switch (demodulatorType) {
        case DemodulatorType::DEMOD_TYPE_FM:
            freqdem_demodulate_block(fdem, &agc_data[0], num_written, &demod_output[0]);
            break;
        case DemodulatorType::DEMOD_TYPE_LSB:
            for (int i = 0; i < num_written; i++) {
                ampmodem_demodulate(ampdem_lsb, resampled_data[i], &demod_output[i]);
            }

            break;
        case DemodulatorType::DEMOD_TYPE_USB:
            for (int i = 0; i < num_written; i++) {
                ampmodem_demodulate(ampdem_usb, resampled_data[i], &demod_output[i]);
            }

            break;
        case DemodulatorType::DEMOD_TYPE_AM:
            am_max = 0;
            for (int i = 0; i < num_written; i++) {
                ampmodem_demodulate(ampdem, resampled_data[i], &demod_output[i]);
                if (demod_output[i] > am_max) {
                    am_max = demod_output[i];
                }
            }
            am_max_ma = am_max_ma + (am_max - am_max_ma) * 0.03;
            am_max_maa = am_max_maa + (am_max_ma - am_max_maa) * 0.03;

            float gain = 0.95/am_max_maa;

            for (int i = 0; i < num_written; i++) {
                demod_output[i] *= gain;
            }

            break;
        }

        if (audio_out_size != resampled_audio_output.size()) {
            if (resampled_audio_output.capacity() < audio_out_size) {
                resampled_audio_output.reserve(audio_out_size);
            }
            resampled_audio_output.resize(audio_out_size);
        }

        unsigned int num_audio_written;
        msresamp_rrrf_execute(audio_resampler, &demod_output[0], num_written, &resampled_audio_output[0], &num_audio_written);

        if (stereo) {
            if (demod_output_stereo.size() != num_written) {
                if (demod_output_stereo.capacity() < num_written) {
                    demod_output_stereo.reserve(num_written);
                }
                demod_output_stereo.resize(num_written);
            }

            double freq = (2.0 * M_PI) * (((double) abs(38000)) / ((double) inp->bandwidth));

            if (shift_freq != freq) {
                nco_crcf_set_frequency(nco_shift, freq);
                shift_freq = freq;
            }

            for (int i = 0; i < num_written; i++) {
                firhilbf_r2c_execute(firR2C, demod_output[i], &x);
                nco_crcf_mix_down(nco_shift, x, &y);
                nco_crcf_step(nco_shift);
                firhilbf_c2r_execute(firC2R, y, &demod_output_stereo[i]);
            }

            if (audio_out_size != resampled_audio_output_stereo.size()) {
                if (resampled_audio_output_stereo.capacity() < audio_out_size) {
                    resampled_audio_output_stereo.reserve(audio_out_size);
                }
                resampled_audio_output_stereo.resize(audio_out_size);
            }

            msresamp_rrrf_execute(stereo_resampler, &demod_output_stereo[0], num_written, &resampled_audio_output_stereo[0], &num_audio_written);
        }

        if (current_level > signal_level) {
            signal_level = signal_level + (current_level - signal_level) * 0.5;
        } else {
            signal_level = signal_level + (current_level - signal_level) * 0.05;
        }

        AudioThreadInput *ati = NULL;

        if (audioInputQueue != NULL) {
            if (!squelch_enabled || (signal_level >= squelch_level)) {

                for (buffers_i = buffers.begin(); buffers_i != buffers.end(); buffers_i++) {
                    if ((*buffers_i)->getRefCount() <= 0) {
                        ati = (*buffers_i);
                        break;
                    }
                }

                if (ati == NULL) {
                    ati = new AudioThreadInput;
                    buffers.push_back(ati);
                }

                ati->setRefCount(1);

                if (stereo) {
                    ati->channels = 2;
                    if (ati->data.capacity() < (num_audio_written * 2)) {
                        ati->data.reserve(num_audio_written * 2);
                    }
                    ati->data.resize(num_audio_written * 2);
                    for (int i = 0; i < num_audio_written; i++) {
                        float l, r;

                        firfilt_rrrf_push(fir_filter, (resampled_audio_output[i] - (resampled_audio_output_stereo[i])));
                        firfilt_rrrf_execute(fir_filter, &l);

                        firfilt_rrrf_push(fir_filter2, (resampled_audio_output[i] + (resampled_audio_output_stereo[i])));
                        firfilt_rrrf_execute(fir_filter2, &r);

                        ati->data[i * 2] = l;
                        ati->data[i * 2 + 1] = r;
                    }
                } else {
                    ati->channels = 1;
                    ati->data.assign(resampled_audio_output.begin(), resampled_audio_output.begin() + num_audio_written);
                }

                audioInputQueue->push(ati);
            }
        }

        if (ati && visOutQueue != NULL && visOutQueue->empty()) {
            AudioThreadInput *ati_vis = new AudioThreadInput;

            int num_vis = DEMOD_VIS_SIZE;
            if (stereo) {
                ati_vis->channels = 2;
                int stereoSize = ati->data.size();
                if (stereoSize > DEMOD_VIS_SIZE) {
                    stereoSize = DEMOD_VIS_SIZE;
                }
                ati_vis->data.resize(stereoSize);

                for (int i = 0; i < stereoSize / 2; i++) {
                    ati_vis->data[i] = ati->data[i * 2];
                    ati_vis->data[i + stereoSize / 2] = ati->data[i * 2 + 1];
                }
            } else {
                ati_vis->channels = 1;
                if (num_audio_written > num_written) {

                    if (num_vis > num_audio_written) {
                        num_vis = num_audio_written;
                    }
                    ati_vis->data.assign(resampled_audio_output.begin(), resampled_audio_output.begin() + num_vis);
                } else {
                    if (num_vis > num_written) {
                        num_vis = num_written;
                    }
                    ati_vis->data.assign(demod_output.begin(), demod_output.begin() + num_vis);
                }

//            std::cout << "Signal: " << agc_crcf_get_signal_level(agc) << " -- " << agc_crcf_get_rssi(agc) << "dB " << std::endl;
            }

            visOutQueue->push(ati_vis);
        }
        if (!threadQueueControl->empty()) {
            while (!threadQueueControl->empty()) {
                DemodulatorThreadControlCommand command;
                threadQueueControl->pop(command);

                switch (command.cmd) {
                case DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_SQUELCH_AUTO:
                    squelch_level = agc_crcf_get_signal_level(agc);
                    squelch_tolerance = agc_crcf_get_signal_level(agc) / 2.0;
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

        inp->decRefCount();
    }

    if (resampler != NULL) {
        msresamp_crcf_destroy(resampler);
    }
    if (audio_resampler != NULL) {
        msresamp_rrrf_destroy(audio_resampler);
    }
    if (stereo_resampler != NULL) {
        msresamp_rrrf_destroy(stereo_resampler);
    }
    if (fir_filter != NULL) {
        firfilt_rrrf_destroy(fir_filter);
    }
    if (fir_filter2 != NULL) {
        firfilt_rrrf_destroy(fir_filter2);
    }

    agc_crcf_destroy(agc);
    firhilbf_destroy(firR2C);
    firhilbf_destroy(firC2R);
    nco_crcf_destroy(nco_shift);

    while (!buffers.empty()) {
        AudioThreadInput *audioDataDel = buffers.front();
        buffers.pop_front();
        delete audioDataDel;
    }

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

void DemodulatorThread::setStereo(bool state) {
    stereo = state;
    std::cout << "Stereo " << (state ? "Enabled" : "Disabled") << std::endl;
}

bool DemodulatorThread::isStereo() {
    return stereo;
}

float DemodulatorThread::getSignalLevel() {
    return signal_level;
}

void DemodulatorThread::setSquelchLevel(float signal_level_in) {
    if (!squelch_enabled) {
        squelch_enabled = true;
    }
    squelch_level = signal_level_in;
}

float DemodulatorThread::getSquelchLevel() {
    return squelch_level;
}

void DemodulatorThread::setDemodulatorType(DemodulatorType demod_type_in) {
    demodulatorType = demod_type_in;
}

DemodulatorType DemodulatorThread::getDemodulatorType() {
    return demodulatorType;
}
