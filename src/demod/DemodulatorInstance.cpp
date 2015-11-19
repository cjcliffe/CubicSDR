#include "DemodulatorInstance.h"

DemodulatorInstance::DemodulatorInstance() :
        t_PreDemod(NULL), t_Demod(NULL), t_Audio(NULL) {

	terminated.store(true);
	audioTerminated.store(true);
	demodTerminated.store(true);
	preDemodTerminated.store(true);
	active.store(false);
	squelch.store(false);
	stereo.store(false);
    muted.store(false);
	tracking.store(false);
	follow.store(false);
	currentAudioSampleRate.store(0);
	currentFrequency.store(0);
	currentBandwidth.store(0);
	currentOutputDevice.store(-1);
    currentAudioGain.store(1.0);

    label = new std::string("Unnamed");
    pipeIQInputData = new DemodulatorThreadInputQueue;
    pipeIQDemodData = new DemodulatorThreadPostInputQueue;
    pipeDemodCommand = new DemodulatorThreadCommandQueue;
    pipeDemodNotify = new DemodulatorThreadCommandQueue;
    
    demodulatorPreThread = new DemodulatorPreThread();
    demodulatorPreThread->setInputQueue("IQDataInput",pipeIQInputData);
    demodulatorPreThread->setOutputQueue("IQDataOutput",pipeIQDemodData);
    demodulatorPreThread->setOutputQueue("NotifyQueue",pipeDemodNotify);
    demodulatorPreThread->setInputQueue("CommandQueue",pipeDemodCommand);
            
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

    currentDemodType = demodulatorPreThread->getParams().demodType;
}

DemodulatorInstance::~DemodulatorInstance() {
    delete audioThread;
    delete demodulatorThread;
    delete demodulatorPreThread;
    delete pipeIQInputData;
    delete pipeIQDemodData;
    delete pipeDemodCommand;
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

//    while (!isTerminated()) {
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }

    currentFrequency = demodulatorPreThread->getParams().frequency;
    currentAudioSampleRate = AudioThread::deviceSampleRate[getOutputDevice()];
    demodulatorPreThread->getParams().audioSampleRate = currentAudioSampleRate;

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

    setDemodulatorType(demodulatorPreThread->getParams().demodType);
    active = true;
    audioTerminated = demodTerminated = preDemodTerminated = terminated = false;

}

void DemodulatorInstance::updateLabel(long long freq) {
    std::stringstream newLabel;
    newLabel.precision(3);
    newLabel << std::fixed << ((long double) freq / 1000000.0);
    setLabel(newLabel.str());
}

DemodulatorThreadCommandQueue *DemodulatorInstance::getCommandQueue() {
    return pipeDemodCommand;
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

bool DemodulatorInstance::isStereo() {
    return stereo;
}

void DemodulatorInstance::setStereo(bool state) {
    stereo = state;
//    demodulatorThread->setStereo(state);
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

void DemodulatorInstance::checkBandwidth() {
//    if ((currentDemodType == DEMOD_TYPE_USB || currentDemodType == DEMOD_TYPE_LSB) && (getBandwidth() % 2)) {
//        setBandwidth(getBandwidth()+1);
//    }
}

void DemodulatorInstance::setDemodulatorType(std::string demod_type_in) {
    currentDemodType = demod_type_in;

    if (currentDemodType == "I/Q") {
        if (currentAudioSampleRate) {
            setBandwidth(currentAudioSampleRate);
        } else {
            setBandwidth(AudioThread::deviceSampleRate[getOutputDevice()]);
        }
    } else if (currentDemodType == "USB" || currentDemodType == "LSB" || currentDemodType == "DSB" || currentDemodType == "AM") {
    	demodulatorThread->setAGC(false);
    } else {
    	demodulatorThread->setAGC(true);
    }
    setGain(getGain());

    demodulatorPreThread->getParams().demodType = currentDemodType;
    if (!active) {
        checkBandwidth();
    } else if (demodulatorThread && threadQueueControl) {
        demodulatorPreThread->setDemodType(currentDemodType);
    }
}

std::string DemodulatorInstance::getDemodulatorType() {
    return currentDemodType;
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

void DemodulatorInstance::setDemodulatorCons(int demod_cons_in) {
    Modem *cModem = demodulatorPreThread->getModem();
    if (cModem && cModem->getType() == "digital") {
        ((ModemDigital *)cModem)->setDemodulatorCons(demod_cons_in);
    }
}

int DemodulatorInstance::getDemodulatorCons() {
    Modem *cModem = demodulatorPreThread->getModem();
    if (cModem && cModem->getType() == "digital") {
        return ((ModemDigital *)cModem)->getDemodulatorCons();
    }
    return 0;
}

void DemodulatorInstance::setBandwidth(int bw) {
    if (currentDemodType == "I/Q") {
        if (currentAudioSampleRate) {
            bw = currentAudioSampleRate;
        } else {
            bw = AudioThread::deviceSampleRate[getOutputDevice()];
        }
    }
    if (!active && demodulatorPreThread != NULL) {
        currentBandwidth = bw;
        checkBandwidth();
        demodulatorPreThread->getParams().bandwidth = currentBandwidth;
    } else if (demodulatorPreThread && pipeDemodCommand) {
        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH;
        currentBandwidth = bw;
        checkBandwidth();
        command.llong_value = currentBandwidth;
        pipeDemodCommand->push(command);
    }
}

int DemodulatorInstance::getBandwidth() {
    if (!currentBandwidth) {
        currentBandwidth = demodulatorPreThread->getParams().bandwidth;
    }
    return currentBandwidth;
}

void DemodulatorInstance::setFrequency(long long freq) {
    if ((freq - getBandwidth() / 2) <= 0) {
        freq = getBandwidth() / 2;
    }
    if (!active) {
        currentFrequency = freq;
        demodulatorPreThread->getParams().frequency = currentFrequency;
    } else if (demodulatorPreThread && pipeDemodCommand) {
        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY;
        currentFrequency = freq;
        command.llong_value = freq;
        pipeDemodCommand->push(command);
    }
}

long long DemodulatorInstance::getFrequency() {
    if (!currentFrequency) {
        currentFrequency = demodulatorPreThread->getParams().frequency;
    }
    return currentFrequency;
}


void DemodulatorInstance::setAudioSampleRate(int sampleRate) {
    if (terminated) {
        currentAudioSampleRate = sampleRate;
        demodulatorPreThread->getParams().audioSampleRate = sampleRate;
    } else if (demodulatorPreThread && pipeDemodCommand) {
        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_AUDIO_RATE;
        currentAudioSampleRate = sampleRate;
        command.llong_value = sampleRate;
        pipeDemodCommand->push(command);
    }
    if (currentDemodType == "I/Q") {
        setBandwidth(currentAudioSampleRate);
    }
}

int DemodulatorInstance::getAudioSampleRate() {
    if (!currentAudioSampleRate) {
        currentAudioSampleRate = audioThread->getSampleRate();
    }
    return currentAudioSampleRate;
}


void DemodulatorInstance::setGain(float gain_in) {
	currentAudioGain = gain_in;

    if (currentDemodType == "I/Q") {
		if (gain_in < 0.25) {
		    audioThread->setGain(1.0);
			demodulatorThread->setAGC(false);
		} else {
		    audioThread->setGain(gain_in);
			demodulatorThread->setAGC(true);
		}
    } else {
        audioThread->setGain(gain_in);
    }
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
