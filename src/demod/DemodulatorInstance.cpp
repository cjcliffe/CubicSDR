#include "DemodulatorInstance.h"

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), t_PreDemod(NULL), t_Audio(NULL), threadQueueDemod(NULL), demodulatorThread(NULL), terminated(false), audioTerminated(false), demodTerminated(
        false), preDemodTerminated(false), active(false), squelch(false), stereo(false) {

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
}

DemodulatorInstance::~DemodulatorInstance() {
    delete audioThread;
    delete demodulatorThread;

    delete audioInputQueue;
    delete threadQueueDemod;
}

void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setVisualOutputQueue(tQueue);
}

void DemodulatorInstance::run() {
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
}

void DemodulatorInstance::updateLabel(int freq) {
    std::stringstream newLabel;
    newLabel.precision(3);
    newLabel << std::fixed << ((float) freq / 1000000.0);
    setLabel(newLabel.str());
}

DemodulatorThreadCommandQueue *DemodulatorInstance::getCommandQueue() {
    return threadQueueCommand;
}

DemodulatorThreadParameters &DemodulatorInstance::getParams() {
    return demodulatorPreThread->getParams();
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
            audioThread = NULL;
            t_Audio->join();
            audioTerminated = true;
            break;
        case DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_TERMINATED:
            demodulatorThread = NULL;
#ifdef __APPLE__
            pthread_join(t_Demod, NULL);
#else
            t_Demod->join();
#endif
            demodTerminated = true;
            break;
        case DemodulatorThreadCommand::DEMOD_THREAD_CMD_DEMOD_PREPROCESS_TERMINATED:
            demodulatorPreThread = NULL;
#ifdef __APPLE__
            pthread_join(t_PreDemod, NULL);
#else
            t_PreDemod->join();
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
    active = state;
    audioThread->setActive(state);
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
    return squelch;
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
    if (audioThread) {
        AudioThreadCommand command;
        command.cmd = AudioThreadCommand::AUDIO_THREAD_CMD_SET_DEVICE;
        command.int_value = device_id;
        audioThread->getCommandQueue()->push(command);
    }
}

int DemodulatorInstance::getOutputDevice() {
    return audioThread->getOutputDevice();
}

void DemodulatorInstance::setDemodulatorType(int demod_type_in) {
    if (demodulatorThread && threadQueueControl) {
        DemodulatorThreadControlCommand command;
         command.cmd = DemodulatorThreadControlCommand::DEMOD_THREAD_CMD_CTL_TYPE;
         currentDemodType = demod_type_in;
         command.demodType = demod_type_in;
         threadQueueControl->push(command);
     }
}

int DemodulatorInstance::getDemodulatorType() {
    return currentDemodType;
}

