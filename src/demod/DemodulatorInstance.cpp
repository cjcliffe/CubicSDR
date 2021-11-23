// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <memory>
#include <iomanip>

#include "DemodulatorInstance.h"
#include "CubicSDR.h"

#include "DemodulatorThread.h"
#include "DemodulatorPreThread.h"
#include "AudioSinkFileThread.h"
#include "AudioFileWAV.h"

#if USE_HAMLIB
#include "RigThread.h"
#endif

DemodVisualCue::DemodVisualCue() {
    squelchBreak.store(false);
}

DemodVisualCue::~DemodVisualCue() = default;

void DemodVisualCue::triggerSquelchBreak(int counter) {
    squelchBreak.store(counter);
}

int DemodVisualCue::getSquelchBreak() {
    return squelchBreak.load();
}

void DemodVisualCue::step() {
    if (squelchBreak.load()) {
        squelchBreak--;
        if (squelchBreak.load() < 0) {
            squelchBreak.store(0);
        }
    }
}

DemodulatorInstance::DemodulatorInstance() {

#if ENABLE_DIGITAL_LAB
    activeOutput = nullptr;
#endif
	
	active.store(false);
    muted.store(false);
    recording.store(false);
    deltaLock.store(false);
    deltaLockOfs.store(0);
	currentOutputDevice.store(-1);
    currentAudioGain.store(1.0);
    follow.store(false);
    tracking.store(false);

    label.store(new std::string("Unnamed"));
    user_label.store(new std::wstring());

    pipeIQInputData = std::make_shared<DemodulatorThreadInputQueue>();
    pipeIQInputData->set_max_num_items(100);
    pipeIQDemodData = std::make_shared< DemodulatorThreadPostInputQueue>();
    pipeIQDemodData->set_max_num_items(100);
    
    audioThread = new AudioThread();
            
    demodulatorPreThread = new DemodulatorPreThread(this);
    demodulatorPreThread->setInputQueue("IQDataInput",pipeIQInputData);
    demodulatorPreThread->setOutputQueue("IQDataOutput",pipeIQDemodData);
            
    pipeAudioData = std::make_shared<AudioThreadInputQueue>();
    pipeAudioData->set_max_num_items(100);

    demodulatorThread = new DemodulatorThread(this);
    demodulatorThread->setInputQueue("IQDataInput",pipeIQDemodData);
    demodulatorThread->setOutputQueue("AudioDataOutput", pipeAudioData);

    audioThread->setInputQueue("AudioDataInput", pipeAudioData);
}

DemodulatorInstance::~DemodulatorInstance() {
    
    std::lock_guard < std::recursive_mutex > lockData(m_thread_control_mutex);

    //now that DemodulatorInstance are managed through shared_ptr, we 
    //should enter here ONLY when it is no longer used by any piece of code, anywhere.
    //so active wait on IsTerminated(), then die.
#define TERMINATION_SPIN_WAIT_MS (20)
#define MAX_WAIT_FOR_TERMINATION_MS (3000.0)
    //this is a stupid busy plus sleep loop
    int  nbCyclesToWait = (MAX_WAIT_FOR_TERMINATION_MS / TERMINATION_SPIN_WAIT_MS) + 1;
    int currentCycle = 0;

    while (currentCycle < nbCyclesToWait) {
        
        if (isTerminated()) {
            std::cout << "Garbage collected demodulator instance '" << getLabel() << "'... " << std::endl << std::flush;

#if ENABLE_DIGITAL_LAB
            delete activeOutput;
#endif           
            delete demodulatorPreThread;
            delete demodulatorThread;
            delete audioThread;
            delete audioSinkThread;

            break;
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(TERMINATION_SPIN_WAIT_MS));
        }
        currentCycle++;
    } //end while
}

void DemodulatorInstance::setVisualOutputQueue(const DemodulatorThreadOutputQueuePtr& tQueue) {
    demodulatorThread->setOutputQueue("AudioVisualOutput", tQueue);
}

void DemodulatorInstance::run() {

    std::lock_guard < std::recursive_mutex > lockData(m_thread_control_mutex);

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

//    std::cout << "Initialized demodulator stack size of " << size << std::endl;

#else
    t_PreDemod = new std::thread(&DemodulatorPreThread::threadMain, demodulatorPreThread);
    t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
#endif

    active = true;
}

void DemodulatorInstance::updateLabel(long long freq) {
    std::stringstream newLabel;
    newLabel.precision(3);
    newLabel << std::fixed << ((long double) freq / 1000000.0);
    setLabel(newLabel.str());
    wxGetApp().getBookmarkMgr().updateActiveList();
}

void DemodulatorInstance::terminate() {

#if ENABLE_DIGITAL_LAB
    if (activeOutput) {
        closeOutput();
    }
#endif

//    std::cout << "Terminating demodulator audio thread.." << std::endl;
    audioThread->terminate();

//    std::cout << "Terminating demodulator thread.." << std::endl;
    demodulatorThread->terminate();
   
//    std::cout << "Terminating demodulator preprocessor thread.." << std::endl;
    demodulatorPreThread->terminate();

    if (audioSinkThread != nullptr) {
        stopRecording();
    }

    //that will actually unblock the currently blocked push().
    pipeIQInputData->flush();
    pipeAudioData->flush();
    pipeIQDemodData->flush();
}

std::string DemodulatorInstance::getLabel() {
    return *(label.load());
}

void DemodulatorInstance::setLabel(std::string labelStr) {
   
    delete label.exchange(new std::string(labelStr));
}

bool DemodulatorInstance::isTerminated() {

    std::lock_guard < std::recursive_mutex > lockData(m_thread_control_mutex);

    bool audioTerminated = audioThread->isTerminated();
    bool demodTerminated = demodulatorThread->isTerminated();
    bool preDemodTerminated = demodulatorPreThread->isTerminated();
    bool audioSinkTerminated = (audioSinkThread == nullptr) || audioSinkThread->isTerminated();

    //Cleanup the worker threads, if the threads are indeed terminated.
    // threads are linked as  t_PreDemod ==> t_Demod ==> t_Audio
    //so terminate in the same order to starve the following threads in succession.
    //i.e waiting on timed-pop so able to se their stopping flag.

    if (preDemodTerminated) {
        
         if (t_PreDemod) {

#ifdef __APPLE__
            pthread_join(t_PreDemod, NULL);
#else
            t_PreDemod->join();
            delete t_PreDemod;
#endif
            t_PreDemod = nullptr;
         }
    }

    if (demodTerminated) {

        if (t_Demod) {
#ifdef __APPLE__
            pthread_join(t_Demod, nullptr);
#else
            t_Demod->join();
            delete t_Demod;
#endif
            t_Demod = nullptr;
        }
    }

    if (audioTerminated) {

        if (t_Audio) {
#ifdef __APPLE__
            pthread_join(t_PreDemod, NULL);
#else
            t_Audio->join();
            delete t_Audio;
#endif
            t_Audio = nullptr;
        }
    }

    if (audioSinkTerminated) {

        if (t_AudioSink != nullptr) {
            t_AudioSink->join();

            delete t_AudioSink;
            t_AudioSink = nullptr;
        }
    }

    bool terminated = audioTerminated && demodTerminated && preDemodTerminated && audioSinkTerminated;

    return terminated;
}

bool DemodulatorInstance::isActive() {
    return active;
}

void DemodulatorInstance::setActive(bool state) {    
    if (active && !state) {
#if ENABLE_DIGITAL_LAB
        if (activeOutput) {
            activeOutput->Hide();
        }
#endif
        audioThread->setActive(state);
  
        DemodulatorThread::releaseSquelchLock(this);

    } else if (!active && state) {
#if ENABLE_DIGITAL_LAB
        if (activeOutput && getModemType() == "digital") {
            activeOutput->Show();
        }
#endif
        audioThread->setActive(state);
    }
    if (!state) {
        tracking = false;
    }
    active = state;
    
    wxGetApp().getBookmarkMgr().updateActiveList();
}

void DemodulatorInstance::squelchAuto() {
    demodulatorThread->setSquelchEnabled(true);
}

bool DemodulatorInstance::isSquelchEnabled() {
    return demodulatorThread->isSquelchEnabled();
}

void DemodulatorInstance::setSquelchEnabled(bool state) {
    demodulatorThread->setSquelchEnabled(state);
}

float DemodulatorInstance::getSignalLevel() {
    return demodulatorThread->getSignalLevel();
}

float DemodulatorInstance::getSignalFloor() {
    return demodulatorThread->getSignalFloor();
}

float DemodulatorInstance::getSignalCeil() {
    return demodulatorThread->getSignalCeil();
}

void DemodulatorInstance::setSquelchLevel(float signal_level_in) {
    demodulatorThread->setSquelchLevel(signal_level_in);
    wxGetApp().getDemodMgr().setLastSquelchLevel(signal_level_in);
    wxGetApp().getDemodMgr().setLastSquelchEnabled(true);
}

float DemodulatorInstance::getSquelchLevel() {
    return demodulatorThread->getSquelchLevel();
}

void DemodulatorInstance::setOutputDevice(int device_id) {
    if (!active) {
        audioThread->setInitOutputDevice(device_id);
    } else if (audioThread) {
        AudioThreadCommand command;
        command.cmdType = AudioThreadCommand::Type::AUDIO_THREAD_CMD_SET_DEVICE;
        command.int_value = device_id;
        //VSO: blocking push
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

void DemodulatorInstance::setDemodulatorType(const std::string& demod_type_in) {
    setGain(getGain());
    if (demodulatorPreThread) {
        std::string currentDemodType = demodulatorPreThread->getDemodType();
        if ((!currentDemodType.empty()) && (currentDemodType != demod_type_in)) {
            lastModemSettings[currentDemodType] = demodulatorPreThread->readModemSettings();
            lastModemBandwidth[currentDemodType] = demodulatorPreThread->getBandwidth();
        }
#if ENABLE_DIGITAL_LAB
        if (activeOutput) {
            activeOutput->Hide();
        }
#endif

        demodulatorPreThread->setDemodType(demod_type_in);
        int lastbw = 0;
        if (!currentDemodType.empty() && lastModemBandwidth.find(demod_type_in) != lastModemBandwidth.end()) {
            lastbw = lastModemBandwidth[demod_type_in];
        }
        if (!lastbw) {
            lastbw = Modem::getModemDefaultSampleRate(demod_type_in);
        }
        if (lastbw) {
            setBandwidth(lastbw);
        }

#if ENABLE_DIGITAL_LAB
        if (isModemInitialized() && getModemType() == "digital") {
            auto *outp = (ModemDigitalOutputConsole *)getOutput();
            outp->setTitle(getDemodulatorType() + ": " + frequencyToStr(getFrequency()));
        }
#endif
    }
    
    wxGetApp().getBookmarkMgr().updateActiveList();
}

std::string DemodulatorInstance::getDemodulatorType() {
    return demodulatorPreThread->getDemodType();
}

std::wstring DemodulatorInstance::getDemodulatorUserLabel() {
    return *(user_label.load());
}

void DemodulatorInstance::setDemodulatorUserLabel(const std::wstring& demod_user_label) {
   
    delete user_label.exchange(new std::wstring(demod_user_label));
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
#if ENABLE_DIGITAL_LAB
    if (activeOutput) {
        if (isModemInitialized() && getModemType() == "digital") {
            auto *outp = (ModemDigitalOutputConsole *)getOutput();
            outp->setTitle(getDemodulatorType() + ": " + frequencyToStr(getFrequency()));
        }
    }
#endif
#if USE_HAMLIB
    if (wxGetApp().rigIsActive() && wxGetApp().getRigThread()->getFollowModem() &&
            wxGetApp().getDemodMgr().getCurrentModem().get() == this) {
        wxGetApp().getRigThread()->setFrequency(freq,true);
    }
#endif
    
    if (this->isActive()) {
        wxGetApp().getBookmarkMgr().updateActiveList();
    }
}

long long DemodulatorInstance::getFrequency() {
    return demodulatorPreThread->getFrequency();
}

void DemodulatorInstance::setAudioSampleRate(int sampleRate) {
    demodulatorPreThread->setAudioSampleRate(sampleRate);
}

int DemodulatorInstance::getAudioSampleRate() const {
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
    return follow.load();
}

void DemodulatorInstance::setFollow(bool follow_in) {
    follow.store(follow_in);
}

bool DemodulatorInstance::isTracking()  {
    return tracking.load();
}

void DemodulatorInstance::setTracking(bool tracking_in) {
    tracking.store(tracking_in);
}

bool DemodulatorInstance::isDeltaLock() {
    return deltaLock.load();
}

void DemodulatorInstance::setDeltaLock(bool lock) {
    deltaLock.store(lock);
}

void DemodulatorInstance::setDeltaLockOfs(int lockOfs) {
    deltaLockOfs.store(lockOfs);
}

int DemodulatorInstance::getDeltaLockOfs() {
    return deltaLockOfs.load();
}

bool DemodulatorInstance::isMuted() {
    return demodulatorThread->isMuted();
}

void DemodulatorInstance::setMuted(bool muted_in) {
    muted = muted_in;
    demodulatorThread->setMuted(muted_in);
    wxGetApp().getDemodMgr().setLastMuted(muted_in);
}

bool DemodulatorInstance::isRecording()
{
    return recording.load();
}

void DemodulatorInstance::setRecording(bool recording_in)
{
    if (recording_in) {
        startRecording();
    }
    else {
        stopRecording();
    }
}

DemodVisualCue *DemodulatorInstance::getVisualCue() {
    return &visualCue;
}

DemodulatorThreadInputQueuePtr DemodulatorInstance::getIQInputDataPipe() {
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

std::string DemodulatorInstance::readModemSetting(const std::string& setting) {
    return demodulatorPreThread->readModemSetting(setting);
}

void DemodulatorInstance::writeModemSetting(const std::string& setting, std::string value) {
    demodulatorPreThread->writeModemSetting(setting, value);
}

ModemSettings DemodulatorInstance::readModemSettings() {
    return demodulatorPreThread->readModemSettings();
}

void DemodulatorInstance::writeModemSettings(ModemSettings settings) {
    demodulatorPreThread->writeModemSettings(settings);
}

bool DemodulatorInstance::isModemInitialized() {
    if (!demodulatorPreThread || isTerminated()) {
        return false;
    }
    return demodulatorPreThread->isInitialized();
}

std::string DemodulatorInstance::getModemType() {
    if (isModemInitialized()) {
        return demodulatorPreThread->getModem()->getType();
    }
    return "";
}

ModemSettings DemodulatorInstance::getLastModemSettings(const std::string& demodType) {
    if (lastModemSettings.find(demodType) != lastModemSettings.end()) {
        return lastModemSettings[demodType];
    } else {
        ModemSettings mods;
        return mods;
    }
}


void DemodulatorInstance::startRecording() {
    if (recording.load()) {
        return;
    }

    auto *newSinkThread = new AudioSinkFileThread();
    auto *afHandler = new AudioFileWAV();

    std::stringstream fileName;
    
    std::wstring userLabel = getDemodulatorUserLabel();

    wxString userLabelForFileName(userLabel);
    std::string userLabelStr = userLabelForFileName.ToStdString();

    if (!userLabelStr.empty()) {
        fileName << userLabelStr;
    } else {
        fileName << getLabel();
    }
   
	newSinkThread->setAudioFileNameBase(fileName.str());

	//attach options:
    newSinkThread->setSquelchOption(wxGetApp().getConfig()->getRecordingSquelchOption());
	newSinkThread->setFileTimeLimit(wxGetApp().getConfig()->getRecordingFileTimeLimit());

    newSinkThread->setAudioFileHandler(afHandler);

    audioSinkThread = newSinkThread;
    t_AudioSink = new std::thread(&AudioSinkThread::threadMain, audioSinkThread);

    demodulatorThread->setOutputQueue("AudioSink", audioSinkThread->getInputQueue("input"));

    recording.store(true);
}


void DemodulatorInstance::stopRecording() {
    if (!recording.load()) {
        return;
    }

    demodulatorThread->setOutputQueue("AudioSink", nullptr);
    audioSinkThread->terminate();
    
    t_AudioSink->join();

    delete t_AudioSink;
    delete audioSinkThread;

    t_AudioSink = nullptr;
    audioSinkThread = nullptr;

    recording.store(false);
}


#if ENABLE_DIGITAL_LAB
ModemDigitalOutput *DemodulatorInstance::getOutput() {
    if (activeOutput == nullptr) {
        activeOutput = new ModemDigitalOutputConsole();
    }
    return activeOutput;
}

void DemodulatorInstance::showOutput() {
    if (activeOutput != nullptr) {
        activeOutput->Show();
    }
}

void DemodulatorInstance::hideOutput() {
    if (activeOutput != nullptr) {
        activeOutput->Hide();
    }
}

void DemodulatorInstance::closeOutput() {
    if (isModemInitialized()) {
        if (getModemType() == "digital") {
            auto *dModem = (ModemDigital *)demodulatorPreThread->getModem();
            dModem->setOutput(nullptr);
        }
    }
    if (activeOutput) {
        activeOutput->Close();
    }
}
#endif
