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

//	stereo.store(false);
    muted.store(false);
	agcEnabled.store(false);
	demodulatorType = "FM";

//    demodFM = freqdem_create(0.5);
//    demodAM_USB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_USB, 1);
//    demodAM_LSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_LSB, 1);
//    demodAM_DSB = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 1);
//    demodAM_DSB_CSP = ampmodem_create(0.5, 0.0, LIQUID_AMPMODEM_DSB, 0);
//    demodAM = demodAM_DSB_CSP;
    
    // advanced demodulators
    
/*    demodulatorCons.store(2);
	currentDemodCons = 0;
	
	demodASK = demodASK2;
    demodASK2 = modem_create(LIQUID_MODEM_ASK2);
    demodASK4 = modem_create(LIQUID_MODEM_ASK4);
    demodASK8 = modem_create(LIQUID_MODEM_ASK8);
    demodASK16 = modem_create(LIQUID_MODEM_ASK16);
    demodASK32 = modem_create(LIQUID_MODEM_ASK32);
    demodASK64 = modem_create(LIQUID_MODEM_ASK64);
    demodASK128 = modem_create(LIQUID_MODEM_ASK128);
    demodASK256 = modem_create(LIQUID_MODEM_ASK256);

	demodAPSK = demodAPSK4;
	demodAPSK4 = modem_create(LIQUID_MODEM_APSK4);
	demodAPSK8 = modem_create(LIQUID_MODEM_APSK8);
	demodAPSK16 = modem_create(LIQUID_MODEM_APSK16);
	demodAPSK32 = modem_create(LIQUID_MODEM_APSK32);
	demodAPSK64 = modem_create(LIQUID_MODEM_APSK64);
	demodAPSK128 = modem_create(LIQUID_MODEM_APSK128);
	demodAPSK256 = modem_create(LIQUID_MODEM_APSK256);
    
    demodBPSK = modem_create(LIQUID_MODEM_BPSK);

	demodDPSK = demodDPSK2;
    demodDPSK2 = modem_create(LIQUID_MODEM_DPSK2);
	demodDPSK4 = modem_create(LIQUID_MODEM_DPSK4);
	demodDPSK8 = modem_create(LIQUID_MODEM_DPSK8);
	demodDPSK16 = modem_create(LIQUID_MODEM_DPSK16);
	demodDPSK32 = modem_create(LIQUID_MODEM_DPSK32);
	demodDPSK64 = modem_create(LIQUID_MODEM_DPSK64);
	demodDPSK128 = modem_create(LIQUID_MODEM_DPSK128);
	demodDPSK256 = modem_create(LIQUID_MODEM_DPSK256);

	demodPSK = demodPSK2;
	demodPSK2 = modem_create(LIQUID_MODEM_PSK2);
	demodPSK4 = modem_create(LIQUID_MODEM_PSK4);
	demodPSK8 = modem_create(LIQUID_MODEM_PSK8);
	demodPSK16 = modem_create(LIQUID_MODEM_PSK16);
	demodPSK32 = modem_create(LIQUID_MODEM_PSK32);
	demodPSK64 = modem_create(LIQUID_MODEM_PSK64);
	demodPSK128 = modem_create(LIQUID_MODEM_PSK128);
	demodPSK256 = modem_create(LIQUID_MODEM_PSK256);

    demodOOK = modem_create(LIQUID_MODEM_OOK);

	demodSQAM = demodSQAM32;
	demodSQAM32 = modem_create(LIQUID_MODEM_SQAM32);
    demodSQAM128 = modem_create(LIQUID_MODEM_SQAM128);

    demodST = modem_create(LIQUID_MODEM_V29);

	demodQAM = demodQAM4;
	demodQAM4 = modem_create(LIQUID_MODEM_QAM4);
	demodQAM8 = modem_create(LIQUID_MODEM_QAM8);
	demodQAM16 = modem_create(LIQUID_MODEM_QAM16);
	demodQAM32 = modem_create(LIQUID_MODEM_QAM32);
	demodQAM64 = modem_create(LIQUID_MODEM_QAM64);
	demodQAM128 = modem_create(LIQUID_MODEM_QAM128);
	demodQAM256 = modem_create(LIQUID_MODEM_QAM256);

    demodQPSK = modem_create(LIQUID_MODEM_QPSK);
    
    currentDemodLock = false; */
    
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

//    msresamp_rrrf audioResampler = NULL;
//    msresamp_rrrf stereoResampler = NULL;
//    firfilt_rrrf firStereoLeft = NULL;
//    firfilt_rrrf firStereoRight = NULL;
//    iirfilt_crcf iirStereoPilot = NULL;

//    liquid_float_complex u, v, w, x, y;

//    firhilbf firStereoR2C = firhilbf_create(5, 60.0f);
//    firhilbf firStereoC2R = firhilbf_create(5, 60.0f);

//    nco_crcf stereoPilot = nco_crcf_create(LIQUID_VCO);
//    nco_crcf_reset(stereoPilot);
//    nco_crcf_pll_set_bandwidth(stereoPilot, 0.25f);

     // half band filter used for side-band elimination
//    resamp2_crcf ssbFilt = resamp2_crcf_create(12,-0.25f,60.0f);

    // Automatic IQ gain
    iqAutoGain = agc_crcf_create();
    agc_crcf_set_bandwidth(iqAutoGain, 0.1);

    ReBuffer<AudioThreadInput> audioVisBuffers;

    std::cout << "Demodulator thread started.." << std::endl;

    iqInputQueue = (DemodulatorThreadPostInputQueue*)getInputQueue("IQDataInput");
    audioOutputQueue = (AudioThreadInputQueue*)getOutputQueue("AudioDataOutput");
    threadQueueControl = (DemodulatorThreadControlCommandQueue *)getInputQueue("ControlQueue");
    threadQueueNotify = (DemodulatorThreadCommandQueue*)getOutputQueue("NotifyQueue");
    
//    switch (demodulatorType.load()) {
//    case DEMOD_TYPE_FM:
//        break;
//    case DEMOD_TYPE_LSB:
//        demodAM = demodAM_LSB;
//        break;
//    case DEMOD_TYPE_USB:
//        demodAM = demodAM_USB;
//        break;
//    case DEMOD_TYPE_DSB:
//        demodAM = demodAM_DSB;
//        break;
//    case DEMOD_TYPE_AM:
//        demodAM = demodAM_DSB_CSP;
//        break;
//    }

    while (!terminated) {
        DemodulatorThreadPostIQData *inp;
        iqInputQueue->pop(inp);
//        std::lock_guard < std::mutex > lock(inp->m_mutex);

        int bufSize = inp->data.size();

        if (!bufSize) {
            inp->decRefCount();
            continue;
        }



        if (agcData.size() != bufSize) {
            if (agcData.capacity() < bufSize) {
                agcData.reserve(bufSize);
                agcAMData.reserve(bufSize);
            }
            agcData.resize(bufSize);
            agcAMData.resize(bufSize);
        }

//        double audio_resample_ratio = inp->audioResampleRatio;

//		if (demodOutputData.size() != bufSize) {
//			if (demodOutputData.capacity() < bufSize) {
//				demodOutputData.reserve(bufSize);
//			}
//			demodOutputData.resize(bufSize);
//		}

/*
		if (demodOutputDataDigital.size() != bufSize) {
			if (demodOutputDataDigital.capacity() < bufSize) {
				demodOutputDataDigital.reserve(bufSize);
			}
			demodOutputDataDigital.resize(bufSize);
		}
*/
//        int audio_out_size = ceil((double) (bufSize) * audio_resample_ratio) + 512;

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

        // Reset demodulator Constellations & Lock
//        updateDemodulatorCons(0);
/*
        if (demodulatorType == DEMOD_TYPE_FM) {
            currentDemodLock = false;
            freqdem_demodulate_block(demodFM, &(*inputData)[0], bufSize, &demodOutputData[0]);
        } else if (demodulatorType == DEMOD_TYPE_RAW) {
            // do nothing here..
        } else {
            switch (demodulatorType.load()) {
            case DEMOD_TYPE_LSB:
				currentDemodLock = false;
                for (int i = 0; i < bufSize; i++) { // Reject upper band
                     resamp2_crcf_filter_execute(ssbFilt,(*inputData)[i],&x,&y);
                     ampmodem_demodulate(demodAM, x, &demodOutputData[i]);
                }
                break;
            case DEMOD_TYPE_USB:
				currentDemodLock = false;
                for (int i = 0; i < bufSize; i++) { // Reject lower band
                    resamp2_crcf_filter_execute(ssbFilt,(*inputData)[i],&x,&y);
                    ampmodem_demodulate(demodAM, y, &demodOutputData[i]);
                }
                break;
            case DEMOD_TYPE_AM:
            case DEMOD_TYPE_DSB:
				currentDemodLock = false;
                for (int i = 0; i < bufSize; i++) {
                    ampmodem_demodulate(demodAM, (*inputData)[i], &demodOutputData[i]);
                }
                break;
			// advanced demodulators
            case DEMOD_TYPE_ASK:

				switch (demodulatorCons.load()) {
				case 2:
                    demodASK = demodASK2;
					updateDemodulatorCons(2);
					break;
				case 4:
					demodASK = demodASK4;
					updateDemodulatorCons(4);
					break;
				case 8:
					demodASK = demodASK8;
					updateDemodulatorCons(8);
					break;
				case 16:
					demodASK = demodASK16;
					updateDemodulatorCons(16);
					break;
				case 32:
					demodASK = demodASK32;
					updateDemodulatorCons(32);
					break;
				case 64:
					demodASK = demodASK64;
					updateDemodulatorCons(64);
					break;
				case 128:
					demodASK = demodASK128;
					updateDemodulatorCons(128);
					break;
				case 256:
					demodASK = demodASK256;
					updateDemodulatorCons(256);
					break;
				default:
					demodASK = demodASK2;
					break;
                }

                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodASK, inp->data[i], &demodOutputDataDigital[i]);
                } 
                updateDemodulatorLock(demodASK, 0.005f);
                break;
			case DEMOD_TYPE_APSK:

				switch (demodulatorCons.load()) {
				case 2:
					demodAPSK = demodAPSK4;
					updateDemodulatorCons(4);
					break;
				case 4:
					demodAPSK = demodAPSK4;
					updateDemodulatorCons(4);
					break;
				case 8:
					demodAPSK = demodAPSK8;
					updateDemodulatorCons(8);
					break;
				case 16:
					demodAPSK = demodAPSK16;
					updateDemodulatorCons(16);
					break;
				case 32:
					demodAPSK = demodAPSK32;
					updateDemodulatorCons(32);
					break;
				case 64:
					demodAPSK = demodAPSK64;
					updateDemodulatorCons(64);
					break;
				case 128:
					demodAPSK = demodAPSK128;
					updateDemodulatorCons(128);
					break;
				case 256:
					demodAPSK = demodAPSK256;
					updateDemodulatorCons(256);
					break;
				default:
					demodAPSK = demodAPSK4;
					break;
				}

				for (int i = 0; i < bufSize; i++) {
					modem_demodulate(demodAPSK, inp->data[i], &demodOutputDataDigital[i]);					
				}
				updateDemodulatorLock(demodAPSK, 0.005f);
				break;
            case DEMOD_TYPE_BPSK:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodBPSK, inp->data[i], &demodOutputDataDigital[i]);                 
                } 
                updateDemodulatorLock(demodBPSK, 0.005f);
                break;
            case DEMOD_TYPE_DPSK:

				switch (demodulatorCons.load()) {
				case 2:
					demodDPSK = demodDPSK2;
					updateDemodulatorCons(2);
					break;
				case 4:
					demodDPSK = demodDPSK4;
					updateDemodulatorCons(4);
					break;
				case 8:
					demodDPSK = demodDPSK8;
					updateDemodulatorCons(8);
					break;
				case 16:
					demodDPSK = demodDPSK16;
					updateDemodulatorCons(16);
					break;
				case 32:
					demodDPSK = demodDPSK32;
					updateDemodulatorCons(32);
					break;
				case 64:
					demodDPSK = demodDPSK64;
					updateDemodulatorCons(64);
					break;
				case 128:
					demodDPSK = demodDPSK128;
					updateDemodulatorCons(128);
					break;
				case 256:
					demodDPSK = demodDPSK256;
					updateDemodulatorCons(256);
					break;
				default:
					demodDPSK = demodDPSK2;
					break;
				}

                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodDPSK, inp->data[i], &demodOutputDataDigital[i]);                 
                } 
                updateDemodulatorLock(demodDPSK, 0.005f);
                break;
            case DEMOD_TYPE_PSK:

				switch (demodulatorCons.load()) {
				case 2:
					demodPSK = demodPSK2;
					updateDemodulatorCons(2);
					break;
				case 4:
					demodPSK = demodPSK4;
					updateDemodulatorCons(4);
					break;
				case 8:
					demodPSK = demodPSK8;
					updateDemodulatorCons(8);
					break;
				case 16:
					demodPSK = demodPSK16;
					updateDemodulatorCons(16);
					break;
				case 32:
					demodPSK = demodPSK32;
					updateDemodulatorCons(32);
					break;
				case 64:
					demodPSK = demodPSK64;
					updateDemodulatorCons(64);
					break;
				case 128:
					demodPSK = demodPSK128;
					updateDemodulatorCons(128);
					break;
				case 256:
					demodPSK = demodPSK256;
					updateDemodulatorCons(256);
					break;
				default:
					demodPSK = demodPSK2;
					break;
				}

                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodPSK, inp->data[i], &demodOutputDataDigital[i]);                
                } 
                updateDemodulatorLock(demodPSK, 0.005f);
                break;
            case DEMOD_TYPE_OOK:
                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodOOK, inp->data[i], &demodOutputDataDigital[i]); 
                } 
                updateDemodulatorLock(demodOOK, 0.005f);
                break;
            case DEMOD_TYPE_SQAM:

				switch (demodulatorCons.load()) {
				case 2:
					demodSQAM = demodSQAM32;
					updateDemodulatorCons(32);
					break;
				case 4:
					demodSQAM = demodSQAM32;
					updateDemodulatorCons(32);
					break;
				case 8:
					demodSQAM = demodSQAM32;
					updateDemodulatorCons(32);
					break;
				case 16:
					demodSQAM = demodSQAM32;
					updateDemodulatorCons(32);
					break;
				case 32:
					demodSQAM = demodSQAM32;
					updateDemodulatorCons(32);
					break;
				case 64:
					demodSQAM = demodSQAM32;
					updateDemodulatorCons(32);
					break;
				case 128:
					demodSQAM = demodSQAM128;
					updateDemodulatorCons(128);
					break;
				case 256:
					demodSQAM = demodSQAM128;
					updateDemodulatorCons(128);
					break;
				default:
					demodSQAM = demodSQAM32;
					break;
				}

                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodSQAM, inp->data[i], &demodOutputDataDigital[i]);                
                } 
                updateDemodulatorLock(demodSQAM, 0.005f);
                break;
            case DEMOD_TYPE_ST:
                for (int i = 0; i < bufSize; i++) {
					modem_demodulate(demodST, inp->data[i], &demodOutputDataDigital[i]);
                } 
                updateDemodulatorLock(demodST, 0.005f);
                break;
            case DEMOD_TYPE_QAM:

				switch (demodulatorCons.load()) {
				case 2:
					demodQAM = demodQAM4;
					updateDemodulatorCons(4);
					break;
				case 4:
					demodQAM = demodQAM4;
					updateDemodulatorCons(4);
					break;
				case 8:
					demodQAM = demodQAM8;
					updateDemodulatorCons(8);
					break;
				case 16:
					demodQAM = demodQAM16;
					updateDemodulatorCons(16);
					break;
				case 32:
					demodQAM = demodQAM32;
					updateDemodulatorCons(32);
					break;
				case 64:
					demodQAM = demodQAM64;
					updateDemodulatorCons(64);
					break;
				case 128:
					demodQAM = demodQAM128;
					updateDemodulatorCons(128);
					break;
				case 256:
					demodQAM = demodQAM256;
					updateDemodulatorCons(256);
					break;
				default:
					demodQAM = demodQAM4;
					break;
				}

                for (int i = 0; i < bufSize; i++) {
                    modem_demodulate(demodQAM, inp->data[i], &demodOutputDataDigital[i]);        
                } 
				updateDemodulatorLock(demodQAM, 0.5f);
                break;
            case DEMOD_TYPE_QPSK:
				for (int i = 0; i < bufSize; i++) {
					modem_demodulate(demodQPSK, inp->data[i], &demodOutputDataDigital[i]);
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

        if (demodulatorType == DEMOD_TYPE_RAW) {
            numAudioWritten = bufSize;
        } else {
        msresamp_rrrf_execute(audioResampler, &demodOutputData[0], bufSize, &resampledOutputData[0], &numAudioWritten);

    }*/

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
                ati->inputRate = inp->sampleRate;
                ati->setRefCount(1);

                /*

                if (demodulatorType == DEMOD_TYPE_RAW) {
                    ati->channels = 2;
                    if (ati->data.capacity() < (numAudioWritten * 2)) {
                        ati->data.reserve(numAudioWritten * 2);
                    }
                    ati->data.resize(numAudioWritten * 2);
                    for (int i = 0; i < numAudioWritten; i++) {
                        ati->data[i * 2] = (*inputData)[i].imag;
                        ati->data[i * 2 + 1] = (*inputData)[i].real;
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
*/
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
/*
        if (ati && audioVisOutputQueue != NULL && audioVisOutputQueue->empty()) {
			AudioThreadInput *ati_vis = audioVisBuffers.getBuffer();
			ati_vis->setRefCount(1);
            ati_vis->sampleRate = inp->sampleRate;
            ati_vis->inputRate = inp->sampleRate;
            
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
                        ati_vis->inputRate = audioSampleRate;
                        ati_vis->sampleRate = 36000;
                        ati_vis->data[i] = ati->data[i * 2];
                        ati_vis->data[i + stereoSize / 2] = ati->data[i * 2 + 1];
                    }
                }
            } else {
                int numAudioWritten = ati->data.size();
                ati_vis->channels = 1;
                if (numAudioWritten > bufSize) {
                    ati_vis->inputRate = audioSampleRate;
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
*/
        
        if (ati != NULL) {
            if (!muted.load()) {
                audioOutputQueue->push(ati);
            } else {
                ati->setRefCount(0);
            }
        }

        if (!threadQueueControl->empty()) {
//            int newDemodType = DEMOD_TYPE_NULL;

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
//                case DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_TYPE:
//                    newDemodType = command.demodType;
//                    break;
                default:
                    break;
                }
            }
/*
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
				case DEMOD_TYPE_ASK:
					//modem_reset(demodASK);
					break;
				case DEMOD_TYPE_APSK:
					//modem_reset(demodAPSK);
					break;
				case DEMOD_TYPE_BPSK:
					//modem_reset(demodBPSK);
					break;
				case DEMOD_TYPE_DPSK:
					//modem_reset(demodDPSK);
					break;
				case DEMOD_TYPE_PSK:
					//modem_reset(demodPSK);
					break;
				case DEMOD_TYPE_OOK:
					//modem_reset(demodOOK);
					break;
				case DEMOD_TYPE_SQAM:
					//modem_reset(demodSQAM);
					break;
				case DEMOD_TYPE_ST:
					//modem_reset(demodST);
					break;
				case DEMOD_TYPE_QAM:
					//modem_reset(demodQAM);
					break;
                case DEMOD_TYPE_QPSK:
                    //modem_reset(demodQPSK);
                    break;
				default:
					// empty default to prevent exceptions
					break;
                }
                demodulatorType = newDemodType;
            }*/
        }

//		demodOutputDataDigital.empty();

        inp->decRefCount();
    }
	// end while !terminated
/*
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
    resamp2_crcf_destroy(ssbFilt);
*/
    outputBuffers.purge();

    if (audioVisOutputQueue && !audioVisOutputQueue->empty()) {
        AudioThreadInput *dummy_vis;
        audioVisOutputQueue->pop(dummy_vis);
    }
	audioVisBuffers.purge();

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

//void DemodulatorThread::setStereo(bool state) {
//    stereo.store(state);
//    std::cout << "Stereo " << (state ? "Enabled" : "Disabled") << std::endl;
//}

//bool DemodulatorThread::isStereo() {
//    return stereo.load();
//}

bool DemodulatorThread::isMuted() {
    return muted.load();
}

void DemodulatorThread::setMuted(bool muted) {
    this->muted.store(muted);
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

void DemodulatorThread::setDemodulatorType(std::string demod_type_in) {
    demodulatorType = demod_type_in;
}

std::string DemodulatorThread::getDemodulatorType() {
    return demodulatorType;
}

void DemodulatorThread::setDemodulatorLock(bool demod_lock_in) {
    demod_lock_in ? currentDemodLock = true : currentDemodLock = false;
}

int DemodulatorThread::getDemodulatorLock() {
    return currentDemodLock;
}

void DemodulatorThread::setDemodulatorCons(int demod_cons_in) {
    demodulatorCons.store(demod_cons_in);
}

int DemodulatorThread::getDemodulatorCons() {
	return currentDemodCons;
}

void DemodulatorThread::updateDemodulatorLock(modem demod, float sensitivity) {
	modem_get_demodulator_evm(demod) <= sensitivity ? setDemodulatorLock(true) : setDemodulatorLock(false);
}

void DemodulatorThread::updateDemodulatorCons(int Cons) {
	if (currentDemodCons != Cons) {
		currentDemodCons = Cons;
	}
}
