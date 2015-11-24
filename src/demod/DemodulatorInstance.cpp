#include "DemodulatorInstance.h"

DemodulatorInstance::DemodulatorInstance() :
        t_PreDemod(NULL), t_Demod(NULL), t_Audio(NULL) {

	terminated.store(true);
	audioTerminated.store(true);
	demodTerminated.store(true);
	preDemodTerminated.store(true);
	active.store(false);
	squelch.store(false);
    muted.store(false);
	tracking.store(false);
	follow.store(false);
	currentOutputDevice.store(-1);
    currentAudioGain.store(1.0);

    label = new std::string("Unnamed");
    pipeIQInputData = new DemodulatorThreadInputQueue;
    pipeIQDemodData = new DemodulatorThreadPostInputQueue;
    pipeDemodNotify = new DemodulatorThreadCommandQueue;
    
    demodulatorPreThread = new DemodulatorPreThread(this);
    demodulatorPreThread->setInputQueue("IQDataInput",pipeIQInputData);
    demodulatorPreThread->setOutputQueue("IQDataOutput",pipeIQDemodData);
    demodulatorPreThread->setOutputQueue("NotifyQueue",pipeDemodNotify);
            
    pipeAudioData = new AudioThreadInputQueue;
    threadQueueControl = new DemodulatorThreadControlCommandQueue;

    demodulatorThread = new DemodulatorThread(this);
    demodulatorThread->setInputQueue("IQDataInput",pipeIQDemodData);
    demodulatorThread->setInputQueue("ControlQueue",threadQueueControl);
    demodulatorThread->setOutputQueue("NotifyQueue",pipeDemodNotify);
    demodulatorThread->setOutputQueue("AudioDataOutput", pipeAudioData);

    audioThread = new AudioThread();
    audioThread->setInputQueue("AudioDataInput", pipeAudioData);
    audioThread->setOutputQueue("NotifyQueue", pipeDemodNotify);
}

DemodulatorInstance::~DemodulatorInstance() {
    delete audioThread;
    delete demodulatorThread;
    delete demodulatorPreThread;
    delete pipeIQInputData;
    delete pipeIQDemodData;
    delete pipeDemodNotify;
    delete threadQueueControl;
    delete pipeAudioData;
}

void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setOutputQueue("AudioVisualOutput", tQueue);
}

void DemodulatorInstance::run() {
    if (active) {
        return;
    }

    t_Audio = new std::thread(&AudioThread::threadMain, audioThread);
    
#ifdef __APPLE__    // Already using pthreads, might as well do some custom init..
    pthread_attr_t attr;
    size_t size;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2048000);
    pthread_attr_getstacksize(&attr, &size);
    pthread_create(&t_PreDemod, &attr, &DemodulatorPreThread::pthread_helper, demodulatorPreThread);
    pthread_attr_destroy(&attr);

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2048000);
    pthread_attr_getstacksize(&attr, &size);
    pthread_create(&t_Demod, &attr, &DemodulatorThread::pthread_helper, demodulatorThread);
    pthread_attr_destroy(&attr);

    std::cout << "Initialized demodulator stack size of " << size << std::endl;

#else
    t_PreDemod = new std::thread(&DemodulatorPreThread::threadMain, demodulatorPreThread);
    t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
#endif

    active = true;
    audioTerminated = demodTerminated = preDemodTerminated = terminated = false;

}

void DemodulatorInstance::updateLabel(long long freq) {
    std::stringstream newLabel;
    newLabel.precision(3);
    newLabel << std::fixed << ((long double) freq / 1000000.0);
    setLabel(newLabel.str());
}

void DemodulatorInstance::terminate() {
    std::cout << "Terminating demodulator audio thread.." << std::endl;
    audioThread->terminate();
    std::cout << "Terminating demodulator thread.." << std::endl;
    demodulatorThread->terminate();
    std::cout << "Terminating demodulator preprocessor thread.." << std::endl;
    demodulatorPreThread->terminate();
}

std::string DemodulatorInstance::getLabel() {
    return *(label.load());
}

void DemodulatorInstance::setLabel(std::string labelStr) {
    std::string *newLabel = new std::string;
    newLabel->append(labelStr);
    std::string *oldLabel;
    oldLabel = label;
    label = newLabel;
    delete oldLabel;
}

bool DemodulatorInstance::isTerminated() {
    while (!pipeDemodNotify->empty()) {
        DemodulatorThreadCommand cmd;
        pipeDemodNotify->pop(cmd);

        switch (cmd.cmd) {
        case DemodulatorThreadCommand::DEMOD_THREAD_CMD_AUDIO_TERMINATED:
            t_Audio->join();
            audioTerminated = true;
            delete t_Audio;
            break;
        case DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED:
#ifdef __APPLE__
            pthread_join(t_Demod, NULL);
#else
            t_Demod->join();
            delete t_Demod;
#endif
            demodTerminated = true;
            break;
        case DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_PREPROCESS_TERMINATED:
#ifdef __APPLE__
            pthread_join(t_PreDemod, NULL);
#else
            t_PreDemod->join();
            delete t_PreDemod;
#endif
            preDemodTerminated = true;
            break;
        default:
            break;
        }
    }

    terminated = audioTerminated && demodTerminated && preDemodTerminated;

    return terminated;
}

bool DemodulatorInstance::isActive() {
    return active;
}

void DemodulatorInstance::setActive(bool state) {
    if (active && !state) {
        audioThread->setActive(state);
    } else if (!active && state) {
        audioThread->setActive(state);
    }
    if (!state) {
        tracking = false;
    }
    active = state;
}

void DemodulatorInstance::squelchAuto() {
    DemodulatorThreadControlCommand command;
    command.cmd = DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_SQUELCH_ON;
    threadQueueControl->push(command);
    squelch = true;
}

bool DemodulatorInstance::isSquelchEnabled() {
    return (demodulatorThread->getSquelchLevel() != 0.0);
}

void DemodulatorInstance::setSquelchEnabled(bool state) {
    if (!state && squelch) {
        DemodulatorThreadControlCommand command;
        command.cmd = DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_SQUELCH_OFF;
        threadQueueControl->push(command);
    } else if (state && !squelch) {
        DemodulatorThreadControlCommand command;
        command.cmd = DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_SQUELCH_ON;
        threadQueueControl->push(command);
    }

    squelch = state;
}

float DemodulatorInstance::getSignalLevel() {
    return demodulatorThread->getSignalLevel();
}

void DemodulatorInstance::setSquelchLevel(float signal_level_in) {
    demodulatorThread->setSquelchLevel(signal_level_in);
}

float DemodulatorInstance::getSquelchLevel() {
    return demodulatorThread->getSquelchLevel();
}

void DemodulatorInstance::setOutputDevice(int device_id) {
    if (!active) {
        audioThread->setInitOutputDevice(device_id);
    } else if (audioThread) {
        AudioThreadCommand command;
        command.cmd = AudioThreadCommand::AUDIO_THREAD_CMD_SET_DEVICE;
        command.int_value = device_id;
        audioThread->getCommandQueue()->push(command);
    }
    setAudioSampleRate(AudioThread::deviceSampleRate[device_id]);
    currentOutputDevice = device_id;
}

int DemodulatorInstance::getOutputDevice() {
    if (currentOutputDevice == -1) {
        if (audioThread) {
            currentOutputDevice = audioThread->getOutputDevice();
        }
    }

    return currentOutputDevice;
}

void DemodulatorInstance::setDemodulatorType(std::string demod_type_in) {
    setGain(getGain());
    if (demodulatorPreThread) {
        demodulatorPreThread->setDemodType(demod_type_in);
    }
}

std::string DemodulatorInstance::getDemodulatorType() {
    return demodulatorPreThread->getDemodType();
}

void DemodulatorInstance::setDemodulatorLock(bool demod_lock_in) {
    Modem *cModem = demodulatorPreThread->getModem();
    if (cModem && cModem->getType() == "digital") {
        ((ModemDigital *)cModem)->setDemodulatorLock(demod_lock_in);
    }
}

int DemodulatorInstance::getDemodulatorLock() {
    Modem *cModem = demodulatorPreThread->getModem();

    if (cModem && cModem->getType() == "digital") {
        return ((ModemDigital *)cModem)->getDemodulatorLock();
    }

    return 0;
}

void DemodulatorInstance::setBandwidth(int bw) {
    demodulatorPreThread->setBandwidth(bw);
}

int DemodulatorInstance::getBandwidth() {
    return demodulatorPreThread->getBandwidth();
}

void DemodulatorInstance::setFrequency(long long freq) {
    if ((freq - getBandwidth() / 2) <= 0) {
        freq = getBandwidth() / 2;
    }
    
    demodulatorPreThread->setFrequency(freq);
}

long long DemodulatorInstance::getFrequency() {
    return demodulatorPreThread->getFrequency();
}

void DemodulatorInstance::setAudioSampleRate(int sampleRate) {
    demodulatorPreThread->setSampleRate(sampleRate);
}

int DemodulatorInstance::getAudioSampleRate() {
    if (!audioThread) {
        return 0;
    }
    return audioThread->getSampleRate();
}


void DemodulatorInstance::setGain(float gain_in) {
	currentAudioGain = gain_in;
    audioThread->setGain(gain_in);
}

float DemodulatorInstance::getGain() {
   return currentAudioGain;
}

bool DemodulatorInstance::isFollow()  {
    return follow;
}

void DemodulatorInstance::setFollow(bool follow) {
    this->follow = follow;
}

bool DemodulatorInstance::isTracking()  {
    return tracking;
}

void DemodulatorInstance::setTracking(bool tracking) {
    this->tracking = tracking;
}

bool DemodulatorInstance::isMuted() {
    return demodulatorThread->isMuted();
}

void DemodulatorInstance::setMuted(bool muted) {
    this->muted = muted;
    demodulatorThread->setMuted(muted);
}

DemodulatorThreadInputQueue *DemodulatorInstance::getIQInputDataPipe() {
    return pipeIQInputData;
}

ModemArgInfoList DemodulatorInstance::getModemArgs() {
    Modem *m = demodulatorPreThread->getModem();
    
    ModemArgInfoList args;
    if (m != nullptr) {
        args = m->getSettings();
    }
    return args;
}

std::string DemodulatorInstance::readModemSetting(std::string setting) {
    Modem *m = demodulatorPreThread->getModem();
    
    if (m) {
        return m->readSetting(setting);
    }
    return "";
}

void DemodulatorInstance::writeModemSetting(std::string setting, std::string value) {
    Modem *m = demodulatorPreThread->getModem();
    
    if (m) {
        m->writeSetting(setting, value);
    }
}
