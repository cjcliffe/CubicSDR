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

DemodulatorThread::DemodulatorThread(DemodulatorThreadPostInputQueue* iqInputQueue, DemodulatorThreadControlCommandQueue *threadQueueControl,
        DemodulatorThreadCommandQueue* threadQueueNotify) :
        iqInputQueue(iqInputQueue), audioVisOutputQueue(NULL), audioOutputQueue(NULL), iqAutoGain(NULL), amOutputCeil(1), amOutputCeilMA(1), amOutputCeilMAA(
                1), stereo(false), terminated(
        false), demodulatorType(DEMOD_TYPE_FM), threadQueueNotify(threadQueueNotify), threadQueueControl(threadQueueControl), squelchLevel(0), signalLevel(
                0), squelchEnabled(false), audioSampleRate(0) {

    demodFM = freqdem_create(0.5);
    demodAM_USB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_USB, 1);
    demodAM_LSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_LSB, 1);
    demodAM_DSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 1);
    demodAM_DSB_CSP = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 0);
    demodAM = demodAM_DSB_CSP;
    
    // advanced demodulators
    // This could properly be done easier.
    
    demodulatorCons = 2;
    
    demodASK = modem_create(LIQUID_MODEM_ASK2);
    demodASK2 = modem_create(LIQUID_MODEM_ASK2);
    demodASK4 = modem_create(LIQUID_MODEM_ASK4);
    demodASK8 = modem_create(LIQUID_MODEM_ASK8);
    demodASK16 = modem_create(LIQUID_MODEM_ASK16);
    demodASK32 = modem_create(LIQUID_MODEM_ASK32);
    demodASK64 = modem_create(LIQUID_MODEM_ASK64);
    demodASK128 = modem_create(LIQUID_MODEM_ASK128);
    demodASK256 = modem_create(LIQUID_MODEM_ASK256);
    
    demodAPSK = modem_create(LIQUID_MODEM_APSK256);
    demodBPSK = modem_create(LIQUID_MODEM_BPSK);
    demodDPSK = modem_create(LIQUID_MODEM_DPSK256);
    demodPSK = modem_create(LIQUID_MODEM_PSK256);
    demodOOK = modem_create(LIQUID_MODEM_OOK);
    demodSQAM = modem_create(LIQUID_MODEM_SQAM128);
    demodST = modem_create(LIQUID_MODEM_V29);
    demodQAM = modem_create(LIQUID_MODEM_QAM256);
    demodQPSK = modem_create(LIQUID_MODEM_QPSK);
    
    currentDemodLock = false;
    
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

    msresamp_rrrf audioResampler = NULL;
    msresamp_rrrf stereoResampler = NULL;
    firfilt_rrrf firStereoLeft = NULL;
    firfilt_rrrf firStereoRight = NULL;

    liquid_float_complex x, y, z[2];
    float rz[2];

    firhilbf firStereoR2C = firhilbf_create(5, 60.0f);
    firhilbf firStereoC2R = firhilbf_create(5, 60.0f);

    nco_crcf stereoShifter = nco_crcf_create(LIQUID_NCO);
    double stereoShiftFrequency = 0;

     // half band filter used for side-band elimination
    resamp2_cccf ssbFilt = resamp2_cccf_create(12,-0.25f,60.0f);

    // Automatic IQ gain
    iqAutoGain = agc_crcf_create();
    agc_crcf_set_bandwidth(iqAutoGain, 0.9);

    AudioThreadInput *ati_vis = new AudioThreadInput;
    ati_vis->data.reserve(DEMOD_VIS_SIZE);

    std::cout << "Demodulator thread started.." << std::endl;

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

    terminated = false;

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

        if (demodulatorType == DEMOD_TYPE_FM) {
            freqdem_demodulate_block(demodFM, &agcData[0], bufSize, &demodOutputData[0]);
        } else {
            float p;
            unsigned int bitstream;
            switch (demodulatorType.load()) {
            case DEMOD_TYPE_LSB:
                for (int i = 0; i < bufSize; i++) { // Reject upper band
                     resamp2_cccf_filter_execute(ssbFilt,inp->data[i],&x,&y);
                     ampmodem_demodulate(demodAM, x, &demodOutputData[i]);
                }
                break;
            case DEMOD_TYPE_USB:
                for (int i = 0; i < bufSize; i++) { // Reject lower band
                    resamp2_cccf_filter_execute(ssbFilt,inp->data[i],&x,&y);
                    ampmodem_demodulate(demodAM, y, &demodOutputData[i]);
                }
                break;
            case DEMOD_TYPE_AM:
            case DEMOD_TYPE_DSB:
                for (int i = 0; i < bufSize; i++) {
                    ampmodem_demodulate(demodAM, inp->data[i], &demodOutputData[i]);
                }
                break;
            case DEMOD_TYPE_ASK:
                if(demodulatorCons == 2) {
                    demodASK = demodASK2;
                }
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodASK, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodASK, 0.5f);
                break;
            case DEMOD_TYPE_BPSK:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodBPSK, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodBPSK, 0.8f);
                break;
            case DEMOD_TYPE_DPSK:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodDPSK, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodDPSK, 0.8f);
                break;
            case DEMOD_TYPE_PSK:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodPSK, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodPSK, 0.8f);
                break;
            case DEMOD_TYPE_OOK:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodOOK, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodOOK, 0.8f);
                break;
            case DEMOD_TYPE_SQAM:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodSQAM, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodSQAM, 0.8f);
                break;
            case DEMOD_TYPE_ST:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodST, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodST, 0.8f);
                break;
            case DEMOD_TYPE_QAM:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodQAM, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodQAM, 0.5f);
                break;
            case DEMOD_TYPE_QPSK:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodQPSK, inp->data[i], &bitstream);
                    // std::cout << bitstream << std::endl;
                } 
                updateDemodulatorLock(demodQPSK, 0.8f);
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
        msresamp_rrrf_execute(audioResampler, &demodOutputData[0], bufSize, &resampledOutputData[0], &numAudioWritten);

        if (stereo) {
            if (demodStereoData.size() != bufSize) {
                if (demodStereoData.capacity() < bufSize) {
                    demodStereoData.reserve(bufSize);
                }
                demodStereoData.resize(bufSize);
            }

            double freq = (2.0 * M_PI) * ((double) 38000) / ((double) inp->sampleRate);

            if (stereoShiftFrequency != freq) {
                nco_crcf_set_frequency(stereoShifter, freq);
                stereoShiftFrequency = freq;
            }

            for (int i = 0; i < bufSize; i++) {
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

            msresamp_rrrf_execute(stereoResampler, &demodStereoData[0], bufSize, &resampledStereoData[0], &numAudioWritten);
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

                ati->sampleRate = audioSampleRate;
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

                std::vector<float>::iterator data_i;
                ati->peak = 0;
                for (data_i = ati->data.begin(); data_i != ati->data.end(); data_i++) {
                    float p = fabs(*data_i);
                    if (p > ati->peak) {
                        ati->peak = p;
                    }
                }

                audioOutputQueue->push(ati);
            }
        }

        if (ati && audioVisOutputQueue != NULL && audioVisOutputQueue->empty()) {

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
//                case DEMOD_TYPE_QPSK:
//                    std::cout << "reset modem qpsk" << std::endl;
//                    modem_reset(demodQPSK);
//                    break;
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

    agc_crcf_destroy(iqAutoGain);
    firhilbf_destroy(firStereoR2C);
    firhilbf_destroy(firStereoC2R);
    nco_crcf_destroy(stereoShifter);
    resamp2_cccf_destroy(ssbFilt);

    while (!outputBuffers.empty()) {
        AudioThreadInput *audioDataDel = outputBuffers.front();
        outputBuffers.pop_front();
        delete audioDataDel;
    }

    if (audioVisOutputQueue && !audioVisOutputQueue->empty()) {
        AudioThreadInput *dummy_vis;
        audioVisOutputQueue->pop(dummy_vis);
    }
    delete ati_vis;

    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
    std::cout << "Demodulator thread done." << std::endl;

#ifdef __APPLE__
    return this;
#endif
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

void DemodulatorThread::setDemodulatorLock(bool demod_lock_in) {
    demod_lock_in ? currentDemodLock = true : currentDemodLock = false;
}

int DemodulatorThread::getDemodulatorLock() {
    return currentDemodLock;
}

void DemodulatorThread::setDemodulatorCons(int demod_cons_in) {
    std::cout << "Updating constellations" << std::endl;
    demodulatorCons = demod_cons_in;
}

int DemodulatorThread::getDemodulatorCons() {
    return demodulatorCons;
}

void DemodulatorThread::updateDemodulatorLock(modem demod, float sensitivity) {
    modem_get_demodulator_evm(demod) <= sensitivity ? setDemodulatorLock(true) : setDemodulatorLock(false);
}

