#include "CubicSDRDefs.h"
#include "DemodulatorThread.h"
#include <vector>

#include <cmath>
#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifdef __APPLE__
#include <pthread.h>
#endif

DemodulatorThread::DemodulatorThread() : IOThread(), iqAutoGain(NULL), amOutputCeil(1), amOutputCeilMA(1), amOutputCeilMAA(1), audioSampleRate(0), squelchLevel(0), signalLevel(0), squelchEnabled(false), iqInputQueue(NULL), audioOutputQueue(NULL), audioVisOutputQueue(NULL), threadQueueControl(NULL), threadQueueNotify(NULL) {

	stereo.store(false);
	agcEnabled.store(false);
	demodulatorType.store(DEMOD_TYPE_FM);

    demodFM = freqdem_create(0.5);
    demodAM_USB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_USB, 1);
    demodAM_LSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_LSB, 1);
    demodAM_DSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 1);
    demodAM_DSB_CSP = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 0);
    demodAM = demodAM_DSB_CSP;

}
DemodulatorThread::~DemodulatorThread() {
}

void DemodulatorThread::onBindOutput(std::string name, ThreadQueueBase *threadQueue) {
    if (name == "AudioVisualOutput") {
        audioVisOutputQueue = (DemodulatorThreadOutputQueue *)threadQueue;
    }
}

void DemodulatorThread::run() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO )-1;
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif

    msresamp_rrrf audioResampler = NULL;
    msresamp_rrrf stereoResampler = NULL;
    firfilt_rrrf firStereoLeft = NULL;
    firfilt_rrrf firStereoRight = NULL;
    iirfilt_crcf iirStereoPilot = NULL;

    liquid_float_complex u, v, w, x, y;

    firhilbf firStereoR2C = firhilbf_create(5, 60.0f);
    firhilbf firStereoC2R = firhilbf_create(5, 60.0f);

    nco_crcf stereoPilot = nco_crcf_create(LIQUID_VCO);
    nco_crcf_reset(stereoPilot);
    nco_crcf_pll_set_bandwidth(stereoPilot, 0.25f);

     // half band filter used for side-band elimination
    resamp2_cccf ssbFilt = resamp2_cccf_create(12,-0.25f,60.0f);

    // Automatic IQ gain
    iqAutoGain = agc_crcf_create();
    agc_crcf_set_bandwidth(iqAutoGain, 0.1);

    AudioThreadInput *ati_vis = new AudioThreadInput;
    ati_vis->data.reserve(DEMOD_VIS_SIZE);

    std::cout << "Demodulator thread started.." << std::endl;

    iqInputQueue = (DemodulatorThreadPostInputQueue*)getInputQueue("IQDataInput");
    audioOutputQueue = (AudioThreadInputQueue*)getOutputQueue("AudioDataOut");
    threadQueueControl = (DemodulatorThreadControlCommandQueue *)getInputQueue("ControlQueue");
    threadQueueNotify = (DemodulatorThreadCommandQueue*)getOutputQueue("NotifyQueue");
    
    switch (demodulatorType.load()) {
    case DEMOD_TYPE_FM:
        break;
    case DEMOD_TYPE_LSB:
        demodAM = demodAM_LSB;
        break;
    case DEMOD_TYPE_USB:
        demodAM = demodAM_USB;
        break;
    case DEMOD_TYPE_DSB:
        demodAM = demodAM_DSB;
        break;
    case DEMOD_TYPE_AM:
        demodAM = demodAM_DSB_CSP;
        break;
    }

    while (!terminated) {
        DemodulatorThreadPostIQData *inp;
        iqInputQueue->pop(inp);
        std::lock_guard < std::mutex > lock(inp->m_mutex);

        int bufSize = inp->data.size();

        if (!bufSize) {
            inp->decRefCount();
            continue;
        }

        if (audioResampler == NULL) {
            audioResampler = inp->audioResampler;
            stereoResampler = inp->stereoResampler;
            firStereoLeft = inp->firStereoLeft;
            firStereoRight = inp->firStereoRight;
            iirStereoPilot = inp->iirStereoPilot;
            audioSampleRate = inp->audioSampleRate;
        } else if (audioResampler != inp->audioResampler) {
            msresamp_rrrf_destroy(audioResampler);
            msresamp_rrrf_destroy(stereoResampler);
            audioResampler = inp->audioResampler;
            stereoResampler = inp->stereoResampler;
            audioSampleRate = inp->audioSampleRate;

            if (demodAM) {
                ampmodem_reset(demodAM);
            }
            freqdem_reset(demodFM);
            nco_crcf_reset(stereoPilot);
        }

        if (firStereoLeft != inp->firStereoLeft) {
            if (firStereoLeft != NULL) {
                firfilt_rrrf_destroy(firStereoLeft);
            }
            firStereoLeft = inp->firStereoLeft;
        }

        if (firStereoRight != inp->firStereoRight) {
            if (firStereoRight != NULL) {
                firfilt_rrrf_destroy(firStereoRight);
            }
            firStereoRight = inp->firStereoRight;
        }

        if (iirStereoPilot != inp->iirStereoPilot) {
            if (iirStereoPilot != NULL) {
                iirfilt_crcf_destroy(iirStereoPilot);
            }
            iirStereoPilot = inp->iirStereoPilot;
        }

        if (agcData.size() != bufSize) {
            if (agcData.capacity() < bufSize) {
                agcData.reserve(bufSize);
                agcAMData.reserve(bufSize);
            }
            agcData.resize(bufSize);
            agcAMData.resize(bufSize);
        }

        double audio_resample_ratio = inp->audioResampleRatio;

        if (demodOutputData.size() != bufSize) {
            if (demodOutputData.capacity() < bufSize) {
                demodOutputData.reserve(bufSize);
            }
            demodOutputData.resize(bufSize);
        }

        int audio_out_size = ceil((double) (bufSize) * audio_resample_ratio) + 512;

        agc_crcf_execute_block(iqAutoGain, &(inp->data[0]), bufSize, &agcData[0]);

        float currentSignalLevel = 0;

        currentSignalLevel = ((60.0 / fabs(agc_crcf_get_rssi(iqAutoGain))) / 15.0 - signalLevel);

        if (agc_crcf_get_signal_level(iqAutoGain) > currentSignalLevel) {
            currentSignalLevel = agc_crcf_get_signal_level(iqAutoGain);
        }

        std::vector<liquid_float_complex> *inputData;

        if (agcEnabled) {
        	inputData = &agcData;
        } else {
        	inputData = &inp->data;
        }

        if (demodulatorType == DEMOD_TYPE_FM) {
            freqdem_demodulate_block(demodFM, &(*inputData)[0], bufSize, &demodOutputData[0]);
        } else if (demodulatorType == DEMOD_TYPE_RAW) {
            // do nothing here..
        } else {
            float p;
            switch (demodulatorType.load()) {
            case DEMOD_TYPE_LSB:
                for (int i = 0; i < bufSize; i++) { // Reject upper band
                     resamp2_cccf_filter_execute(ssbFilt,(*inputData)[i],&x,&y);
                     ampmodem_demodulate(demodAM, x, &demodOutputData[i]);
                }
                break;
            case DEMOD_TYPE_USB:
                for (int i = 0; i < bufSize; i++) { // Reject lower band
                    resamp2_cccf_filter_execute(ssbFilt,(*inputData)[i],&x,&y);
                    ampmodem_demodulate(demodAM, y, &demodOutputData[i]);
                }
                break;
            case DEMOD_TYPE_AM:
            case DEMOD_TYPE_DSB:
                for (int i = 0; i < bufSize; i++) {
                    ampmodem_demodulate(demodAM, (*inputData)[i], &demodOutputData[i]);
                }
                break;
            }

            amOutputCeilMA = amOutputCeilMA + (amOutputCeil - amOutputCeilMA) * 0.025;
            amOutputCeilMAA = amOutputCeilMAA + (amOutputCeilMA - amOutputCeilMAA) * 0.025;

            amOutputCeil = 0;

            for (int i = 0; i < bufSize; i++) {
                if (demodOutputData[i] > amOutputCeil) {
                    amOutputCeil = demodOutputData[i];
                }
            }

            float gain = 0.5 / amOutputCeilMAA;

            for (int i = 0; i < bufSize; i++) {
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

        if (demodulatorType == DEMOD_TYPE_RAW) {
            numAudioWritten = bufSize;
        } else {
        msresamp_rrrf_execute(audioResampler, &demodOutputData[0], bufSize, &resampledOutputData[0], &numAudioWritten);

        if (stereo && inp->sampleRate >= 100000) {
            if (demodStereoData.size() != bufSize) {
                if (demodStereoData.capacity() < bufSize) {
                    demodStereoData.reserve(bufSize);
                }
                demodStereoData.resize(bufSize);
            }

            
            float phase_error = 0;

            for (int i = 0; i < bufSize; i++) {
                // real -> complex
                firhilbf_r2c_execute(firStereoR2C, demodOutputData[i], &x);

                // 19khz pilot band-pass
                iirfilt_crcf_execute(iirStereoPilot, x, &v);
                nco_crcf_cexpf(stereoPilot, &w);
                                
                w.imag = -w.imag; // conjf(w)
                
                // multiply u = v * conjf(w)
                u.real = v.real * w.real - v.imag * w.imag;
                u.imag = v.real * w.imag + v.imag * w.real;
                
                // cargf(u)
                phase_error = atan2f(u.imag,u.real);
                
                // step pll
                nco_crcf_pll_step(stereoPilot, phase_error);
                nco_crcf_step(stereoPilot);
                
                // 38khz down-mix
                nco_crcf_mix_down(stereoPilot, x, &y);
                nco_crcf_mix_down(stereoPilot, y, &x);

                // complex -> real
                firhilbf_c2r_execute(firStereoC2R, x, &demodStereoData[i]);
            }

//            std::cout << "[PLL] phase error: " << phase_error;
//            std::cout << " freq:" << (((nco_crcf_get_frequency(stereoPilot) / (2.0 * M_PI)) * inp->sampleRate)) << std::endl;
            
            if (audio_out_size != resampledStereoData.size()) {
                if (resampledStereoData.capacity() < audio_out_size) {
                    resampledStereoData.reserve(audio_out_size);
                }
                resampledStereoData.resize(audio_out_size);
            }

            msresamp_rrrf_execute(stereoResampler, &demodStereoData[0], bufSize, &resampledStereoData[0], &numAudioWritten);
        }
        }

        if (currentSignalLevel > signalLevel) {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.5;
        } else {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.05;
        }

        AudioThreadInput *ati = NULL;

        if (audioOutputQueue != NULL) {
            if (!squelchEnabled || (signalLevel >= squelchLevel)) {

                ati = outputBuffers.getBuffer();

                ati->sampleRate = audioSampleRate;
                ati->setRefCount(1);

                if (demodulatorType == DEMOD_TYPE_RAW) {
                    ati->channels = 2;
                    if (ati->data.capacity() < (numAudioWritten * 2)) {
                        ati->data.reserve(numAudioWritten * 2);
                    }
                    ati->data.resize(numAudioWritten * 2);
                    for (int i = 0; i < numAudioWritten; i++) {
                        ati->data[i * 2] = (*inputData)[i].real;
                        ati->data[i * 2 + 1] = (*inputData)[i].imag;
                    }
                } else if (stereo && inp->sampleRate >= 100000) {
                    ati->channels = 2;
                    if (ati->data.capacity() < (numAudioWritten * 2)) {
                        ati->data.reserve(numAudioWritten * 2);
                    }
                    ati->data.resize(numAudioWritten * 2);
                    for (int i = 0; i < numAudioWritten; i++) {
                        float l, r;

                        firfilt_rrrf_push(firStereoLeft, 0.568 * (resampledOutputData[i] - (resampledStereoData[i])));
                        firfilt_rrrf_execute(firStereoLeft, &l);

                        firfilt_rrrf_push(firStereoRight, 0.568 * (resampledOutputData[i] + (resampledStereoData[i])));
                        firfilt_rrrf_execute(firStereoRight, &r);

                        ati->data[i * 2] = l;
                        ati->data[i * 2 + 1] = r;
                    }
                } else {
                    ati->channels = 1;
                    ati->data.assign(resampledOutputData.begin(), resampledOutputData.begin() + numAudioWritten);
                }

                std::vector<float>::iterator data_i;
                ati->peak = 0;
                for (data_i = ati->data.begin(); data_i != ati->data.end(); data_i++) {
                    float p = fabs(*data_i);
                    if (p > ati->peak) {
                        ati->peak = p;
                    }
                }
            }
        }

        if (ati && audioVisOutputQueue != NULL && audioVisOutputQueue->empty()) {

            ati_vis->busy_update.lock();

            int num_vis = DEMOD_VIS_SIZE;
            if (demodulatorType == DEMOD_TYPE_RAW || (stereo && inp->sampleRate >= 100000)) {
                ati_vis->channels = 2;
                int stereoSize = ati->data.size();
                if (stereoSize > DEMOD_VIS_SIZE * 2) {
                    stereoSize = DEMOD_VIS_SIZE * 2;
                }

                ati_vis->data.resize(stereoSize);

                if (demodulatorType == DEMOD_TYPE_RAW) {
                    for (int i = 0; i < stereoSize / 2; i++) {
                        ati_vis->data[i] = agcData[i].real * 0.75;
                        ati_vis->data[i + stereoSize / 2] = agcData[i].imag * 0.75;
                    }
                } else {
                    for (int i = 0; i < stereoSize / 2; i++) {
                        ati_vis->data[i] = ati->data[i * 2];
                        ati_vis->data[i + stereoSize / 2] = ati->data[i * 2 + 1];
                    }
                }
            } else {
                ati_vis->channels = 1;
                if (numAudioWritten > bufSize) {

                    if (num_vis > numAudioWritten) {
                        num_vis = numAudioWritten;
                    }
                    ati_vis->data.assign(resampledOutputData.begin(), resampledOutputData.begin() + num_vis);
                } else {
                    if (num_vis > bufSize) {
                        num_vis = bufSize;
                    }
                    ati_vis->data.assign(demodOutputData.begin(), demodOutputData.begin() + num_vis);
                }

//            std::cout << "Signal: " << agc_crcf_get_signal_level(agc) << " -- " << agc_crcf_get_rssi(agc) << "dB " << std::endl;
            }

            audioVisOutputQueue->push(ati_vis);

            ati_vis->busy_update.unlock();
        }

        if (ati != NULL) {
            audioOutputQueue->push(ati);
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
                    freqdem_reset(demodFM);
                    break;
                case DEMOD_TYPE_LSB:
                    demodAM = demodAM_LSB;
                    ampmodem_reset(demodAM);
                    break;
                case DEMOD_TYPE_USB:
                    demodAM = demodAM_USB;
                    ampmodem_reset(demodAM);
                    break;
                case DEMOD_TYPE_DSB:
                    demodAM = demodAM_DSB;
                    ampmodem_reset(demodAM);
                    break;
                case DEMOD_TYPE_AM:
                    demodAM = demodAM_DSB_CSP;
                    ampmodem_reset(demodAM);
                    break;
                }
                demodulatorType = newDemodType;
            }
        }

        inp->decRefCount();
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
    if (iirStereoPilot != NULL) {
      iirfilt_crcf_destroy(iirStereoPilot);
    }

    agc_crcf_destroy(iqAutoGain);
    firhilbf_destroy(firStereoR2C);
    firhilbf_destroy(firStereoC2R);
    nco_crcf_destroy(stereoPilot);
    resamp2_cccf_destroy(ssbFilt);

    outputBuffers.purge();

    if (audioVisOutputQueue && !audioVisOutputQueue->empty()) {
        AudioThreadInput *dummy_vis;
        audioVisOutputQueue->pop(dummy_vis);
    }
    delete ati_vis;

    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
    
    std::cout << "Demodulator thread done." << std::endl;
}

void DemodulatorThread::terminate() {
    terminated = true;
    DemodulatorThreadPostIQData *inp = new DemodulatorThreadPostIQData;    // push dummy to nudge queue
    iqInputQueue->push(inp);
}

void DemodulatorThread::setStereo(bool state) {
    stereo.store(state);
    std::cout << "Stereo " << (state ? "Enabled" : "Disabled") << std::endl;
}

bool DemodulatorThread::isStereo() {
    return stereo.load();
}

void DemodulatorThread::setAGC(bool state) {
	agcEnabled.store(state);
}

bool DemodulatorThread::getAGC() {
	return agcEnabled.load();
}


float DemodulatorThread::getSignalLevel() {
    return signalLevel.load();
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

