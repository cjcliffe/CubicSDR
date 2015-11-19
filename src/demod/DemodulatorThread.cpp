#include "CubicSDRDefs.h"
#include "DemodulatorThread.h"
#include "DemodulatorInstance.h"
#include <vector>

#include <cmath>
#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifdef __APPLE__
#include <pthread.h>
#endif

DemodulatorThread::DemodulatorThread(DemodulatorInstance *parent) : IOThread(), iqAutoGain(NULL), audioSampleRate(0), squelchLevel(0), signalLevel(0), squelchEnabled(false), iqInputQueue(NULL), audioOutputQueue(NULL), audioVisOutputQueue(NULL), threadQueueControl(NULL), threadQueueNotify(NULL), cModem(nullptr), cModemKit(nullptr) {
    
    demodInstance = parent;
    muted.store(false);
    agcEnabled.store(false);
    
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
    
    // Automatic IQ gain
    iqAutoGain = agc_crcf_create();
    agc_crcf_set_bandwidth(iqAutoGain, 0.1);
    
    ReBuffer<AudioThreadInput> audioVisBuffers;
    
    std::cout << "Demodulator thread started.." << std::endl;
    
    iqInputQueue = (DemodulatorThreadPostInputQueue*)getInputQueue("IQDataInput");
    audioOutputQueue = (AudioThreadInputQueue*)getOutputQueue("AudioDataOutput");
    threadQueueControl = (DemodulatorThreadControlCommandQueue *)getInputQueue("ControlQueue");
    threadQueueNotify = (DemodulatorThreadCommandQueue*)getOutputQueue("NotifyQueue");
    
    ModemIQData modemData;
    
    while (!terminated) {
        DemodulatorThreadPostIQData *inp;
        iqInputQueue->pop(inp);
        //        std::lock_guard < std::mutex > lock(inp->m_mutex);
        
        audioSampleRate = demodInstance->getAudioSampleRate();
        
        int bufSize = inp->data.size();
        
        if (!bufSize) {
            inp->decRefCount();
            continue;
        }
        
        if (inp->modemKit && inp->modemKit != cModemKit) {
            if (cModemKit != nullptr) {
                cModem->disposeKit(cModemKit);
            }
            cModemKit = inp->modemKit;
        }
        
        if (inp->modem && inp->modem != cModem) {
            delete cModem;
            cModem = inp->modem;
        }
        
        if (!cModem || !cModemKit) {
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
        
        modemData.sampleRate = inp->sampleRate;
        modemData.data.assign(inputData->begin(), inputData->end());
        modemData.setRefCount(1);
        
        AudioThreadInput *ati = NULL;
        ati = outputBuffers.getBuffer();
        
        ati->sampleRate = audioSampleRate;
        ati->inputRate = inp->sampleRate;
        ati->setRefCount(1);
        
        cModem->demodulate(cModemKit, &modemData, ati);
        
        if (currentSignalLevel > signalLevel) {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.5;
        } else {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.05;
        }
        
        if (audioOutputQueue != NULL) {
            if (ati && (!squelchEnabled || (signalLevel >= squelchLevel))) {
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
            AudioThreadInput *ati_vis = audioVisBuffers.getBuffer();
            ati_vis->setRefCount(1);
            ati_vis->sampleRate = inp->sampleRate;
            ati_vis->inputRate = inp->sampleRate;
            
            int num_vis = DEMOD_VIS_SIZE;
            if (ati->channels==2) {
                ati_vis->channels = 2;
                int stereoSize = ati->data.size();
                if (stereoSize > DEMOD_VIS_SIZE * 2) {
                    stereoSize = DEMOD_VIS_SIZE * 2;
                }
                
                ati_vis->data.resize(stereoSize);
                
                if (inp->modemType == "I/Q") {
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
                //                if (numAudioWritten > bufSize) {
                ati_vis->inputRate = audioSampleRate;
                if (num_vis > numAudioWritten) {
                    num_vis = numAudioWritten;
                }
                ati_vis->data.assign(ati->data.begin(), ati->data.begin() + num_vis);
                //                } else {
                //                    if (num_vis > bufSize) {
                //                        num_vis = bufSize;
                //                    }
                //                    ati_vis->data.assign(ati->data.begin(), ati->data.begin() + num_vis);
                //                }
                
                //            std::cout << "Signal: " << agc_crcf_get_signal_level(agc) << " -- " << agc_crcf_get_rssi(agc) << "dB " << std::endl;
            }
            
            audioVisOutputQueue->push(ati_vis);
        }
        
        
        if (ati != NULL) {
            if (!muted.load()) {
                audioOutputQueue->push(ati);
            } else {
                ati->setRefCount(0);
            }
        }
        
        if (!threadQueueControl->empty()) {
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
                    default:
                        break;
                }
            }
        }
        
        inp->decRefCount();
    }
    // end while !terminated
    
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
