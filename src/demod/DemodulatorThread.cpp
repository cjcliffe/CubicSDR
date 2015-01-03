#include "DemodulatorThread.h"
#include "CubicSDRDefs.h"
#include <vector>

#ifdef __APPLE__
#include <pthread.h>
#endif

DemodulatorThread::DemodulatorThread(DemodulatorThreadPostInputQueue* iqInputQueue, DemodulatorThreadControlCommandQueue *threadQueueControl,
        DemodulatorThreadCommandQueue* threadQueueNotify) :
        iqInputQueue(iqInputQueue), audioVisOutputQueue(NULL), audioOutputQueue(NULL), iqAutoGain(NULL), amOutputCeil(1), amOutputCeilMA(1), amOutputCeilMAA(
                1), stereo(false), terminated(
        false), demodulatorType(DEMOD_TYPE_FM), threadQueueNotify(threadQueueNotify), threadQueueControl(threadQueueControl), squelchLevel(0), signalLevel(
                0), squelchEnabled(false) {

    demodFM = freqdem_create(0.5);
    demodAM_USB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_LSB, 1);
    demodAM_LSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_USB, 1);
    demodAM_DSB_CSP = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 0);
    demodAM = demodAM_DSB_CSP;

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

    msresamp_crcf resampler = NULL;
    msresamp_rrrf audioResampler = NULL;
    msresamp_rrrf stereoResampler = NULL;
    firfilt_rrrf firStereoLeft = NULL;
    firfilt_rrrf firStereoRight = NULL;

    // Stereo filters / shifters
    double firStereoCutoff = 0.5 * ((double) 36000 / (double) AUDIO_FREQUENCY);         // filter cutoff frequency
    float ft = 0.05f;         // filter transition
    float As = 120.0f;         // stop-band attenuation [dB]
    float mu = 0.0f;         // fractional timing offset

    if (firStereoCutoff < 0) {
        firStereoCutoff = 0;
    }

    if (firStereoCutoff > 0.5) {
        firStereoCutoff = 0.5;
    }

    unsigned int h_len = estimate_req_filter_len(ft, As);
    float h[h_len];
    liquid_firdes_kaiser(h_len, firStereoCutoff, As, mu, h);

    firStereoLeft = firfilt_rrrf_create(h, h_len);
    firStereoRight = firfilt_rrrf_create(h, h_len);

    liquid_float_complex x, y;

    firhilbf firStereoR2C = firhilbf_create(5, 60.0f);
    firhilbf firStereoC2R = firhilbf_create(5, 60.0f);

    nco_crcf stereoShifter = nco_crcf_create(LIQUID_NCO);
    double stereoShiftFrequency = 0;

    // SSB Half-band filter
    nco_crcf ssbShifterUp = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_frequency(ssbShifterUp, (2.0 * M_PI) * 0.25);

    nco_crcf ssbShifterDown = nco_crcf_create(LIQUID_NCO);
    nco_crcf_set_frequency(ssbShifterDown, (2.0 * M_PI) * 0.25);

    float ssbFt = 0.001f;         // filter transition
    float ssbAs = 120.0f;         // stop-band attenuation [dB]

    h_len = estimate_req_filter_len(ssbFt, ssbAs);
    float ssb_h[h_len];
    liquid_firdes_kaiser(h_len, 0.25, ssbAs, 0.0, ssb_h);

    firfilt_crcf firSSB = firfilt_crcf_create(ssb_h, h_len);

    // Automatic IQ gain
    iqAutoGain = agc_crcf_create();
    agc_crcf_set_bandwidth(iqAutoGain, 0.9);

    std::cout << "Demodulator thread started.." << std::endl;

    while (!terminated) {
        DemodulatorThreadPostIQData *inp;
        iqInputQueue->pop(inp);
        std::lock_guard < std::mutex > lock(inp->m_mutex);

        int bufSize = inp->data.size();

        if (!bufSize) {
            inp->decRefCount();
            continue;
        }

        if (resampler == NULL) {
            resampler = inp->resampler;
            audioResampler = inp->audioResampler;
            stereoResampler = inp->stereoResampler;
        } else if (resampler != inp->resampler) {
            msresamp_crcf_destroy(resampler);
            msresamp_rrrf_destroy(audioResampler);
            msresamp_rrrf_destroy(stereoResampler);
            resampler = inp->resampler;
            audioResampler = inp->audioResampler;
            stereoResampler = inp->stereoResampler;

            ampmodem_reset(demodAM_USB);
            ampmodem_reset(demodAM_LSB);
            ampmodem_reset(demodAM_DSB_CSP);
            freqdem_reset(demodFM);
        }

        int out_size = ceil((double) (bufSize) * inp->resamplerRatio) + 512;

        if (agcData.size() != out_size) {
            if (agcData.capacity() < out_size) {
                agcData.reserve(out_size);
                agcAMData.reserve(out_size);
                resampledData.reserve(out_size);
            }
            agcData.resize(out_size);
            resampledData.resize(out_size);
            agcAMData.resize(out_size);
        }

        unsigned int numWritten;
        msresamp_crcf_execute(resampler, &(inp->data[0]), bufSize, &resampledData[0], &numWritten);

        double audio_resample_ratio = inp->audioResampleRatio;

        if (demodOutputData.size() != numWritten) {
            if (demodOutputData.capacity() < numWritten) {
                demodOutputData.reserve(numWritten);
            }
            demodOutputData.resize(numWritten);
        }

        int audio_out_size = ceil((double) (numWritten) * audio_resample_ratio) + 512;

        agc_crcf_execute_block(iqAutoGain, &resampledData[0], numWritten, &agcData[0]);

        float currentSignalLevel = 0;

        currentSignalLevel = ((60.0 / fabs(agc_crcf_get_rssi(iqAutoGain))) / 15.0 - signalLevel);

        if (agc_crcf_get_signal_level(iqAutoGain) > currentSignalLevel) {
            currentSignalLevel = agc_crcf_get_signal_level(iqAutoGain);
        }

        if (demodulatorType == DEMOD_TYPE_FM) {
            freqdem_demodulate_block(demodFM, &agcData[0], numWritten, &demodOutputData[0]);
        } else {
            float p;
            switch (demodulatorType) {
            case DEMOD_TYPE_LSB:
                for (int i = 0; i < numWritten; i++) { // Reject upper band
                    nco_crcf_mix_up(ssbShifterUp, resampledData[i], &x);
                    nco_crcf_step(ssbShifterUp);
                    firfilt_crcf_push(firSSB, x);
                    firfilt_crcf_execute(firSSB, &x);
                    nco_crcf_mix_down(ssbShifterDown, x, &resampledData[i]);
                    nco_crcf_step(ssbShifterDown);
                }
                break;
            case DEMOD_TYPE_USB:
                for (int i = 0; i < numWritten; i++) { // Reject lower band
                    nco_crcf_mix_down(ssbShifterDown, resampledData[i], &x);
                    nco_crcf_step(ssbShifterDown);
                    firfilt_crcf_push(firSSB, x);
                    firfilt_crcf_execute(firSSB, &x);
                    nco_crcf_mix_up(ssbShifterUp, x, &resampledData[i]);
                    nco_crcf_step(ssbShifterUp);
                }
                break;
            case DEMOD_TYPE_AM:
                break;
            }

            amOutputCeil = 0;

            for (int i = 0; i < numWritten; i++) {
                ampmodem_demodulate(demodAM, resampledData[i], &demodOutputData[i]);
                if (demodOutputData[i] > amOutputCeil) {
                    amOutputCeil = demodOutputData[i];
                }
            }
            amOutputCeilMA = amOutputCeilMA + (amOutputCeil - amOutputCeilMA) * 0.05;
            amOutputCeilMAA = amOutputCeilMAA + (amOutputCeilMA - amOutputCeilMAA) * 0.05;

            float gain = 0.95 / amOutputCeilMAA;

            for (int i = 0; i < numWritten; i++) {
                demodOutputData[i] *= gain;
            }
        }

        if (audio_out_size != resampledOutputData.size()) {
            if (resampledOutputData.capacity() < audio_out_size) {
                resampledOutputData.reserve(audio_out_size);
            }
            resampledOutputData.resize(audio_out_size);
        }

        unsigned int numAudioWritten;
        msresamp_rrrf_execute(audioResampler, &demodOutputData[0], numWritten, &resampledOutputData[0], &numAudioWritten);

        if (stereo) {
            if (demodStereoData.size() != numWritten) {
                if (demodStereoData.capacity() < numWritten) {
                    demodStereoData.reserve(numWritten);
                }
                demodStereoData.resize(numWritten);
            }

            double freq = (2.0 * M_PI) * (((double) abs(38000)) / ((double) inp->bandwidth));

            if (stereoShiftFrequency != freq) {
                nco_crcf_set_frequency(stereoShifter, freq);
                stereoShiftFrequency = freq;
            }

            for (int i = 0; i < numWritten; i++) {
                firhilbf_r2c_execute(firStereoR2C, demodOutputData[i], &x);
                nco_crcf_mix_down(stereoShifter, x, &y);
                nco_crcf_step(stereoShifter);
                firhilbf_c2r_execute(firStereoC2R, y, &demodStereoData[i]);
            }

            if (audio_out_size != resampledStereoData.size()) {
                if (resampledStereoData.capacity() < audio_out_size) {
                    resampledStereoData.reserve(audio_out_size);
                }
                resampledStereoData.resize(audio_out_size);
            }

            msresamp_rrrf_execute(stereoResampler, &demodStereoData[0], numWritten, &resampledStereoData[0], &numAudioWritten);
        }

        if (currentSignalLevel > signalLevel) {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.5;
        } else {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.05;
        }

        AudioThreadInput *ati = NULL;

        if (audioOutputQueue != NULL) {
            if (!squelchEnabled || (signalLevel >= squelchLevel)) {

                for (outputBuffersI = outputBuffers.begin(); outputBuffersI != outputBuffers.end(); outputBuffersI++) {
                    if ((*outputBuffersI)->getRefCount() <= 0) {
                        ati = (*outputBuffersI);
                        break;
                    }
                }

                if (ati == NULL) {
                    ati = new AudioThreadInput;
                    outputBuffers.push_back(ati);
                }

                ati->setRefCount(1);

                if (stereo) {
                    ati->channels = 2;
                    if (ati->data.capacity() < (numAudioWritten * 2)) {
                        ati->data.reserve(numAudioWritten * 2);
                    }
                    ati->data.resize(numAudioWritten * 2);
                    for (int i = 0; i < numAudioWritten; i++) {
                        float l, r;

                        firfilt_rrrf_push(firStereoLeft, (resampledOutputData[i] - (resampledStereoData[i])));
                        firfilt_rrrf_execute(firStereoLeft, &l);

                        firfilt_rrrf_push(firStereoRight, (resampledOutputData[i] + (resampledStereoData[i])));
                        firfilt_rrrf_execute(firStereoRight, &r);

                        ati->data[i * 2] = l;
                        ati->data[i * 2 + 1] = r;
                    }
                } else {
                    ati->channels = 1;
                    ati->data.assign(resampledOutputData.begin(), resampledOutputData.begin() + numAudioWritten);
                }

                audioOutputQueue->push(ati);
            }
        }

        if (ati && audioVisOutputQueue != NULL && audioVisOutputQueue->empty()) {
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
                if (numAudioWritten > numWritten) {

                    if (num_vis > numAudioWritten) {
                        num_vis = numAudioWritten;
                    }
                    ati_vis->data.assign(resampledOutputData.begin(), resampledOutputData.begin() + num_vis);
                } else {
                    if (num_vis > numWritten) {
                        num_vis = numWritten;
                    }
                    ati_vis->data.assign(demodOutputData.begin(), demodOutputData.begin() + num_vis);
                }

//            std::cout << "Signal: " << agc_crcf_get_signal_level(agc) << " -- " << agc_crcf_get_rssi(agc) << "dB " << std::endl;
            }

            audioVisOutputQueue->push(ati_vis);
        }
        if (!threadQueueControl->empty()) {
            int newDemodType = DEMOD_TYPE_NULL;

            while (!threadQueueControl->empty()) {
                DemodulatorThreadControlCommand command;
                threadQueueControl->pop(command);

                switch (command.cmd) {
                case DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_SQUELCH_ON:
                    squelchEnabled = true;
                    break;
                case DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_SQUELCH_OFF:
                    squelchEnabled = false;
                    break;
                case DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_TYPE:
                    newDemodType = command.demodType;
                    break;
                default:
                    break;
                }
            }

            if (newDemodType != DEMOD_TYPE_NULL) {
                switch (newDemodType) {
                case DEMOD_TYPE_FM:
                    break;
                case DEMOD_TYPE_LSB:
                    demodAM = demodAM_USB;
                    break;
                case DEMOD_TYPE_USB:
                    demodAM = demodAM_LSB;
                    break;
                case DEMOD_TYPE_AM:
                    demodAM = demodAM_DSB_CSP;
                    break;
                }
                demodulatorType = newDemodType;
            }
        }

        inp->decRefCount();
    }

    if (resampler != NULL) {
        msresamp_crcf_destroy(resampler);
    }
    if (audioResampler != NULL) {
        msresamp_rrrf_destroy(audioResampler);
    }
    if (stereoResampler != NULL) {
        msresamp_rrrf_destroy(stereoResampler);
    }
    if (firStereoLeft != NULL) {
        firfilt_rrrf_destroy(firStereoLeft);
    }
    if (firStereoRight != NULL) {
        firfilt_rrrf_destroy(firStereoRight);
    }

    agc_crcf_destroy(iqAutoGain);
    firhilbf_destroy(firStereoR2C);
    firhilbf_destroy(firStereoC2R);
    nco_crcf_destroy(stereoShifter);
    nco_crcf_destroy(ssbShifterUp);
    nco_crcf_destroy(ssbShifterDown);

    while (!outputBuffers.empty()) {
        AudioThreadInput *audioDataDel = outputBuffers.front();
        outputBuffers.pop_front();
        delete audioDataDel;
    }

    std::cout << "Demodulator thread done." << std::endl;
    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
}

void DemodulatorThread::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    audioVisOutputQueue = tQueue;
}

void DemodulatorThread::setAudioOutputQueue(AudioThreadInputQueue *tQueue) {
    audioOutputQueue = tQueue;
}

void DemodulatorThread::terminate() {
    terminated = true;
    DemodulatorThreadPostIQData *inp = new DemodulatorThreadPostIQData;    // push dummy to nudge queue
    iqInputQueue->push(inp);
}

void DemodulatorThread::setStereo(bool state) {
    stereo = state;
    std::cout << "Stereo " << (state ? "Enabled" : "Disabled") << std::endl;
}

bool DemodulatorThread::isStereo() {
    return stereo;
}

float DemodulatorThread::getSignalLevel() {
    return signalLevel;
}

void DemodulatorThread::setSquelchLevel(float signal_level_in) {
    if (!squelchEnabled) {
        squelchEnabled = true;
    }
    squelchLevel = signal_level_in;
}

float DemodulatorThread::getSquelchLevel() {
    return squelchLevel;
}

void DemodulatorThread::setDemodulatorType(int demod_type_in) {
    demodulatorType = demod_type_in;
}

int DemodulatorThread::getDemodulatorType() {
    return demodulatorType;
}

