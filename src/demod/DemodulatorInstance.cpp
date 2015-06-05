#include "DemodulatorInstance.h"

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), t_PreDemod(NULL), t_Audio(NULL), threadQueueDemod(NULL), demodulatorThread(NULL), terminated(true), audioTerminated(true), demodTerminated(
        true), preDemodTerminated(true), active(false), squelch(false), stereo(false), currentFrequency(0), currentBandwidth(0), currentOutputDevice(-1) {

    label = new std::string("Unnamed");
    threadQueueDemod = new DemodulatorThreadInputQueue;
    threadQueuePostDemod = new DemodulatorThreadPostInputQueue;
    threadQueueCommand = new DemodulatorThreadCommandQueue;
    threadQueueNotify = new DemodulatorThreadCommandQueue;
    threadQueueControl = new DemodulatorThreadControlCommandQueue;

    demodulatorPreThread = new DemodulatorPreThread(threadQueueDemod, threadQueuePostDemod, threadQueueControl, threadQueueNotify);
    demodulatorPreThread->setCommandQueue(threadQueueCommand);
    demodulatorThread = new DemodulatorThread(threadQueuePostDemod, threadQueueControl, threadQueueNotify);

    audioInputQueue = new AudioThreadInputQueue;
    audioThread = new AudioThread(audioInputQueue, threadQueueNotify);

    demodulatorThread->setAudioOutputQueue(audioInputQueue);

    currentDemodType = demodulatorThread->getDemodulatorType();
    currentDemodLock = demodulatorThread->getDemodulatorLock();
}

DemodulatorInstance::~DemodulatorInstance() {
    delete audioThread;
    delete demodulatorThread;
    delete demodulatorPreThread;
    delete threadQueueDemod;
    delete threadQueuePostDemod;
    delete threadQueueCommand;
    delete threadQueueNotify;
    delete threadQueueControl;
    delete audioInputQueue;
}

void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setVisualOutputQueue(tQueue);
}

void DemodulatorInstance::run() {
    if (active) {
        return;
    }

//    while (!isTerminated()) {
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }

    currentFrequency = demodulatorPreThread->getParams().frequency;
    currentDemodType = demodulatorThread->getDemodulatorType();
    currentDemodLock = demodulatorThread->getDemodulatorLock();
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
    return threadQueueCommand;
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
    while (!threadQueueNotify->empty()) {
        DemodulatorThreadCommand cmd;
        threadQueueNotify->pop(cmd);

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
    demodulatorThread->setStereo(state);
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
        setAudioSampleRate(AudioThread::deviceSampleRate[device_id]);

        AudioThreadCommand command;
        command.cmd = AudioThreadCommand::AUDIO_THREAD_CMD_SET_DEVICE;
        command.int_value = device_id;
        audioThread->getCommandQueue()->push(command);
    }
    currentOutputDevice = device_id;
}

int DemodulatorInstance::getOutputDevice() {
    if (currentOutputDevice == -1) {
        currentOutputDevice = audioThread->getOutputDevice();
    }

    return currentOutputDevice;
}

void DemodulatorInstance::checkBandwidth() {
//    if ((currentDemodType == DEMOD_TYPE_USB || currentDemodType == DEMOD_TYPE_LSB) && (getBandwidth() % 2)) {
//        setBandwidth(getBandwidth()+1);
//    }
}

void DemodulatorInstance::setDemodulatorType(int demod_type_in) {
    if (!active) {
        currentDemodType = demod_type_in;
        checkBandwidth();
        demodulatorPreThread->getParams().demodType = currentDemodType;
        demodulatorThread->setDemodulatorType(currentDemodType);
    } else if (demodulatorThread && threadQueueControl) {
        DemodulatorThreadControlCommand command;
        command.cmd = DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_TYPE;
        currentDemodType = demod_type_in;
        command.demodType = demod_type_in;
        checkBandwidth();
        threadQueueControl->push(command);
    }
}

int DemodulatorInstance::getDemodulatorType() {
    return currentDemodType;
}

void DemodulatorInstance::setDemodulatorLock(bool demod_lock_in) {
    if(demod_lock_in) {
        currentDemodLock = true;
    }
    else {
        currentDemodLock = false;
    }
}

int DemodulatorInstance::getDemodulatorLock() {
    return currentDemodLock;
}

void DemodulatorInstance::setBandwidth(int bw) {
    if (!active) {
        currentBandwidth = bw;
        checkBandwidth();
        demodulatorPreThread->getParams().bandwidth = currentBandwidth;
    } else if (demodulatorPreThread && threadQueueCommand) {
        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH;
        currentBandwidth = bw;
        checkBandwidth();
        command.llong_value = currentBandwidth;
        threadQueueCommand->push(command);
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
    } else if (demodulatorPreThread && threadQueueCommand) {
        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY;
        currentFrequency = freq;
        command.llong_value = freq;
        threadQueueCommand->push(command);
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
    } else if (demodulatorPreThread && threadQueueCommand) {
        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_AUDIO_RATE;
        currentAudioSampleRate = sampleRate;
        command.llong_value = sampleRate;
        threadQueueCommand->push(command);
    }
}

int DemodulatorInstance::getAudioSampleRate() {
    if (!currentAudioSampleRate) {
        currentAudioSampleRate = audioThread->getSampleRate();
    }
    return currentAudioSampleRate;
}


void DemodulatorInstance::setGain(float gain_in) {
    audioThread->setGain(gain_in);
}

float DemodulatorInstance::getGain() {
   return audioThread->getGain();
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
