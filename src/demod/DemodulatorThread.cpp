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
      signalLevel(-100), signalFloor(-30), signalCeil(30), squelchEnabled(false) {
    
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
     
    ModemIQData modemData;
    
    while (!stopping) {
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
        
        std::vector<liquid_float_complex> *inputData;
        
        inputData = &inp->data;
        
        modemData.sampleRate = inp->sampleRate;
        modemData.data.assign(inputData->begin(), inputData->end());
        
        AudioThreadInput *ati = nullptr;
        
        ModemAnalog *modemAnalog = (cModem->getType() == "analog")?((ModemAnalog *)cModem):nullptr;
        ModemDigital *modemDigital = (cModem->getType() == "digital")?((ModemDigital *)cModem):nullptr;
        
        if (modemAnalog != nullptr) {
            ati = outputBuffers.getBuffer();
            
            ati->sampleRate = cModemKit->audioSampleRate;
            ati->inputRate = inp->sampleRate;
        } else if (modemDigital != nullptr) {
            ati = outputBuffers.getBuffer();
            
            ati->sampleRate = cModemKit->sampleRate;
            ati->inputRate = inp->sampleRate;
            ati->data.resize(0);
        }

        cModem->demodulate(cModemKit, &modemData, ati);

        float currentSignalLevel = 0;
        float sampleTime = float(inp->data.size()) / float(inp->sampleRate);

        if (audioOutputQueue != nullptr && ati && ati->data.size()) {
            float accum = 0;

            for (auto i : ati->data) {
                accum += abMagnitude(0.948059448969, 0.392699081699, i, 0.0);
            }

            float audioSignalLevel = linearToDb(accum / float(ati->data.size()));

            accum = 0;
            
            for (auto i : inp->data) {
                accum += abMagnitude(0.948059448969, 0.392699081699, i.real, i.imag);
            }

            float iqSignalLevel = linearToDb(accum / float(inp->data.size()));
            
            currentSignalLevel = iqSignalLevel>audioSignalLevel?iqSignalLevel:audioSignalLevel;
            
            float sf = signalFloor.load(), sc = signalCeil.load(), sl = squelchLevel.load();
            
            if (currentSignalLevel > sc) {
                sc = currentSignalLevel;
            }
            
            if (currentSignalLevel < sf) {
                sf = currentSignalLevel;
            }
            
            if (sl+1.0f > sc) {
                sc = sl+1.0f;
            }
            
            if ((sf+2.0f) > sc) {
                sc = sf+2.0f;
            }
            
            sc -= (sc - (currentSignalLevel + 2.0f)) * sampleTime * 0.05f;
            sf += ((currentSignalLevel - 5.0f) - sf) * sampleTime * 0.15f;
            
            signalFloor.store(sf);
            signalCeil.store(sc);
        }
        
        if (currentSignalLevel > signalLevel) {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.5;
        } else {
            signalLevel = signalLevel + (currentSignalLevel - signalLevel) * 0.05 * sampleTime * 30.0;
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
        
        if (audioOutputQueue != nullptr && ati && ati->data.size() && !squelched) {
            std::vector<float>::iterator data_i;
            ati->peak = 0;
            for (auto data_i : ati->data) {
                float p = fabs(data_i);
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

        if ((ati || modemDigital) && localAudioVisOutputQueue != nullptr && localAudioVisOutputQueue->empty()) {
            AudioThreadInput *ati_vis = new AudioThreadInput;

            ati_vis->sampleRate = inp->sampleRate;
            ati_vis->inputRate = inp->sampleRate;
            
            size_t num_vis = DEMOD_VIS_SIZE;
            if (modemDigital) {
                if (ati) {  // TODO: handle digital modems with audio output
                    ati->setRefCount(0);
                    ati = nullptr;
                }
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
            
            if (!localAudioVisOutputQueue->push(ati_vis)) {
                ati_vis->setRefCount(0);
                std::cout << "DemodulatorThread::run() cannot push ati_vis into localAudioVisOutputQueue, is full !" << std::endl;
                std::this_thread::yield();
            }
        }
        
        
        if (ati != nullptr) {
            if (!muted.load() && (!wxGetApp().getSoloMode() || (demodInstance == wxGetApp().getDemodMgr().getLastActiveDemodulator()))) {
                
                if (!audioOutputQueue->push(ati)) {
                    ati->decRefCount();
                    std::cout << "DemodulatorThread::run() cannot push ati into audioOutputQueue, is full !" << std::endl;
                    std::this_thread::yield();
                }

            } else {
                ati->setRefCount(0);
            }
        }
        
        DemodulatorThreadControlCommand command;
        
        //empty command queue, execute commands
        while (threadQueueControl->try_pop(command)) {
                       
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
        
        
        inp->decRefCount();
    }
    // end while !stopping
    
    // Purge any unused inputs, with a non-blocking pop
    DemodulatorThreadPostIQData *ref;
    while (iqInputQueue->try_pop(ref)) {
        
        if (ref) {  // May have other consumers; just decrement
            ref->decRefCount();
        }
    }

    AudioThreadInput *ref_audio;
    while (audioOutputQueue->try_pop(ref_audio)) {
      
        if (ref_audio) { // Originated here; set RefCount to 0
            ref_audio->setRefCount(0);
        }
    }

    outputBuffers.purge();
    
//    std::cout << "Demodulator thread done." << std::endl;
}

void DemodulatorThread::terminate() {
    IOThread::terminate();
    DemodulatorThreadPostIQData *inp = new DemodulatorThreadPostIQData;    // push dummy to nudge queue
    if (!iqInputQueue->push(inp)) {
        delete inp;
    }
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

float DemodulatorThread::getSignalFloor() {
    return signalFloor.load();
}

float DemodulatorThread::getSignalCeil() {
    return signalCeil.load();
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
