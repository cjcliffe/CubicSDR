#include "CubicSDRDefs.h"
#include "DemodulatorThread.h"
#include "DemodulatorInstance.h"
#include "CubicSDR.h"
#include <vector>

#include <cmath>
#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifdef __APPLE__
#include <pthread.h>
#endif

DemodulatorThread::DemodulatorThread(DemodulatorInstance *parent) 
    : IOThread(), outputBuffers("DemodulatorThreadBuffers"), squelchLevel(-100), 
      signalLevel(-100), squelchEnabled(false) {
    
    demodInstance = parent;
    muted.store(false);
    squelchBreak = false;
}

DemodulatorThread::~DemodulatorThread() {
}

void DemodulatorThread::onBindOutput(std::string name, ThreadQueueBase *threadQueue) {
    if (name == "AudioVisualOutput") {
        
        //protects because it may be changed at runtime
        std::lock_guard < std::mutex > lock(m_mutexAudioVisOutputQueue);

        audioVisOutputQueue = static_cast<DemodulatorThreadOutputQueue*>(threadQueue);
    }
}

float DemodulatorThread::abMagnitude(double alpha, double beta, float inphase, float quadrature) {
    //        http://dspguru.com/dsp/tricks/magnitude-estimator
    /* magnitude ~= alpha * max(|I|, |Q|) + beta * min(|I|, |Q|) */
    double abs_inphase = fabs(inphase);
    double abs_quadrature = fabs(quadrature);
    if (abs_inphase > abs_quadrature) {
        return alpha * abs_inphase + beta * abs_quadrature;
    } else {
        return alpha * abs_quadrature + beta * abs_inphase;
    }
}

float DemodulatorThread::linearToDb(float linear) {
    //        http://dspguru.com/dsp/tricks/magnitude-estimator
    #define SMALL 1e-20
    if (linear <= SMALL) {
        linear = float(SMALL);
    }
    return 20.0 * log10(linear);
}

void DemodulatorThread::run() {
#ifdef __APPLE__
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO )-1;
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif
    
//    std::cout << "Demodulator thread started.." << std::endl;
    
    iqInputQueue = static_cast<DemodulatorThreadPostInputQueue*>(getInputQueue("IQDataInput"));
    audioOutputQueue = static_cast<AudioThreadInputQueue*>(getOutputQueue("AudioDataOutput"));
    threadQueueControl = static_cast<DemodulatorThreadControlCommandQueue *>(getInputQueue("ControlQueue"));
    threadQueueNotify = static_cast<DemodulatorThreadCommandQueue*>(getOutputQueue("NotifyQueue"));
    
    ModemIQData modemData;
    
    while (!terminated) {
        DemodulatorThreadPostIQData *inp;
        iqInputQueue->pop(inp);
        //        std::lock_guard < std::mutex > lock(inp->m_mutex);
        
        size_t bufSize = inp->data.size();
        
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
        
        float currentSignalLevel = 0;
        float accum = 0;
        
        for (std::vector<liquid_float_complex>::iterator i = inp->data.begin(); i != inp->data.end(); i++) {
            accum += abMagnitude(0.948059448969, 0.392699081699, i->real, i->imag);
        }
        
        currentSignalLevel = linearToDb(accum / float(inp->data.size()));
        if (currentSignalLevel < DEMOD_SIGNAL_MIN+1) {
            currentSignalLevel = DEMOD_SIGNAL_MIN+1;
        }
        
        std::vector<liquid_float_complex> *inputData;
        
        inputData = &inp->data;
        
        modemData.sampleRate = inp->sampleRate;
        modemData.data.assign(inputData->begin(), inputData->end());
        modemData.setRefCount(1);
        
        AudioThreadInput *ati = nullptr;
        
        ModemAnalog *modemAnalog = (cModem->getType() == "analog")?((ModemAnalog *)cModem):nullptr;
        ModemDigital *modemDigital = (cModem->getType() == "digital")?((ModemDigital *)cModem):nullptr;
        
        if (modemAnalog != nullptr) {
            ati = outputBuffers.getBuffer();
            
            ati->sampleRate = cModemKit->audioSampleRate;
            ati->inputRate = inp->sampleRate;
            ati->setRefCount(1);
        } else if (modemDigital != nullptr) {
            ati = outputBuffers.getBuffer();
            
            ati->sampleRate = cModemKit->sampleRate;
            ati->inputRate = inp->sampleRate;
            ati->setRefCount(1);
        }

        cModem->demodulate(cModemKit, &modemData, ati);
        
        if (currentSignalLevel > signalLevel) {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.5;
        } else {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.05;
        }
        
        bool squelched = (squelchEnabled && (signalLevel < squelchLevel));
        
        if (squelchEnabled) {
            if (!squelched && !squelchBreak) {
                if (wxGetApp().getSoloMode() && !muted.load()) {
                    wxGetApp().getDemodMgr().setActiveDemodulator(demodInstance, false);
                }
                squelchBreak = true;
                demodInstance->getVisualCue()->triggerSquelchBreak(120);
            } else if (squelched && squelchBreak) {
                squelchBreak = false;
            }
        }
        
        if (audioOutputQueue != nullptr && ati && !squelched) {
            std::vector<float>::iterator data_i;
            ati->peak = 0;
            for (data_i = ati->data.begin(); data_i != ati->data.end(); data_i++) {
                float p = fabs(*data_i);
                if (p > ati->peak) {
                    ati->peak = p;
                }
            }
        } else if (ati) {
            ati->setRefCount(0);
            ati = nullptr;
        }
        
        //At that point, capture the current state of audioVisOutputQueue in a local 
        //variable, and works with it with now on until the next while-turn.
        DemodulatorThreadOutputQueue* localAudioVisOutputQueue = nullptr;
        {
            std::lock_guard < std::mutex > lock(m_mutexAudioVisOutputQueue);
            localAudioVisOutputQueue = audioVisOutputQueue;
        }

        if (ati && localAudioVisOutputQueue != nullptr && localAudioVisOutputQueue->empty()) {
            AudioThreadInput *ati_vis = new AudioThreadInput;

            ati_vis->setRefCount(1);
            ati_vis->sampleRate = inp->sampleRate;
            ati_vis->inputRate = inp->sampleRate;
            
            size_t num_vis = DEMOD_VIS_SIZE;
            if (modemDigital) {
                ati_vis->data.resize(inputData->size());
                ati_vis->channels = 2;
                for (int i = 0, iMax = inputData->size() / 2; i < iMax; i++) {
                    ati_vis->data[i * 2] = (*inputData)[i].real;
                    ati_vis->data[i * 2 + 1] = (*inputData)[i].imag;
                }
                ati_vis->type = 2;
            } else if (ati->channels==2) {
                ati_vis->channels = 2;
                int stereoSize = ati->data.size();
                if (stereoSize > DEMOD_VIS_SIZE * 2) {
                    stereoSize = DEMOD_VIS_SIZE * 2;
                }
                
                ati_vis->data.resize(stereoSize);
                
                if (inp->modemName == "I/Q") {
                    for (int i = 0; i < stereoSize / 2; i++) {
                        ati_vis->data[i] = (*inputData)[i].real * 0.75;
                        ati_vis->data[i + stereoSize / 2] = (*inputData)[i].imag * 0.75;
                    }
                } else {
                    for (int i = 0; i < stereoSize / 2; i++) {
                        ati_vis->inputRate = cModemKit->audioSampleRate;
                        ati_vis->sampleRate = 36000;
                        ati_vis->data[i] = ati->data[i * 2];
                        ati_vis->data[i + stereoSize / 2] = ati->data[i * 2 + 1];
                    }
                }
                ati_vis->type = 1;
            } else {
                size_t numAudioWritten = ati->data.size();
                ati_vis->channels = 1;
                std::vector<float> *demodOutData = (modemAnalog != nullptr)?modemAnalog->getDemodOutputData():nullptr;
                if ((numAudioWritten > bufSize) || (demodOutData == nullptr)) {
                    ati_vis->inputRate = cModemKit->audioSampleRate;
                    if (num_vis > numAudioWritten) {
                        num_vis = numAudioWritten;
                    }
                    ati_vis->data.assign(ati->data.begin(), ati->data.begin() + num_vis);
                } else {
                    if (num_vis > demodOutData->size()) {
                        num_vis = demodOutData->size();
                    }
                    ati_vis->data.assign(demodOutData->begin(), demodOutData->begin() + num_vis);
                }
                ati_vis->type = 0;
            }
            
            localAudioVisOutputQueue->push(ati_vis);
        }
        
        
        if (ati != nullptr) {
            if (!muted.load() && (!wxGetApp().getSoloMode() || (demodInstance == wxGetApp().getDemodMgr().getLastActiveDemodulator()))) {
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
    
    // Purge any unused inputs
    while (!iqInputQueue->empty()) {
        DemodulatorThreadPostIQData *ref;
        iqInputQueue->pop(ref);
        if (ref) {
            ref->setRefCount(0);
        }
    }
    while (!audioOutputQueue->empty()) {
        AudioThreadInput *ref;
        audioOutputQueue->pop(ref);
        if (ref) {
            ref->setRefCount(0);
        }
    }
    outputBuffers.purge();
    
    //Guard the cleanup of audioVisOutputQueue properly.
    std::lock_guard < std::mutex > lock(m_mutexAudioVisOutputQueue);
    
    DemodulatorThreadCommand tCmd(DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED);
    tCmd.context = this;
    threadQueueNotify->push(tCmd);
    
//    std::cout << "Demodulator thread done." << std::endl;
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
