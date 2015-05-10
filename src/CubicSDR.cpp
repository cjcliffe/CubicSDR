#define OPENGL

#include "CubicSDRDefs.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "FrequencyDialog.h"

#ifdef _OSX_APP_
#include "CoreFoundation/CoreFoundation.h"
#endif

IMPLEMENT_APP(CubicSDR)

bool CubicSDR::OnInit() {
#ifdef _OSX_APP_
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
    {
        // error!
    }
    CFRelease(resourcesURL);
    chdir(path);
#endif

    if (!wxApp::OnInit()) {
        return false;
    }

    wxApp::SetAppName("CubicSDR");

    config.load();

    frequency = DEFAULT_FREQ;
    offset = 0;
    ppm = 0;

    audioVisualQueue = new DemodulatorThreadOutputQueue();
    audioVisualQueue->set_max_num_items(1);

    threadCmdQueueSDR = new SDRThreadCommandQueue;
    sdrThread = new SDRThread(threadCmdQueueSDR);

    sdrPostThread = new SDRPostThread();
    sdrPostThread->setNumVisSamples(16384 * 2);

    iqPostDataQueue = new SDRThreadIQDataQueue;
    iqVisualQueue = new DemodulatorThreadInputQueue;
    iqVisualQueue->set_max_num_items(1);

    sdrThread->setIQDataOutQueue(iqPostDataQueue);
    sdrPostThread->setIQDataInQueue(iqPostDataQueue);
    sdrPostThread->setIQVisualQueue(iqVisualQueue);

    std::vector<SDRDeviceInfo *>::iterator devs_i;

    SDRThread::enumerate_rtl(&devs);

    if (devs.size() > 1) {
        wxArrayString choices;
        for (devs_i = devs.begin(); devs_i != devs.end(); devs_i++) {
            std::string devName = (*devs_i)->getName();
            if ((*devs_i)->isAvailable()) {
                devName.append(": ");
                devName.append((*devs_i)->getProduct());
                devName.append(" [");
                devName.append((*devs_i)->getSerial());
                devName.append("]");
            } else {
                devName.append(" (In Use?)");
            }
            choices.Add(devName);
        }

        int devId = wxGetSingleChoiceIndex(wxT("Devices"), wxT("Choose Input Device"), choices);

        std::cout << "Chosen: " << devId << std::endl;
        sdrThread->setDeviceId(devId);
    }

    t_PostSDR = new std::thread(&SDRPostThread::threadMain, sdrPostThread);
    t_SDR = new std::thread(&SDRThread::threadMain, sdrThread);

    appframe = new AppFrame();

#ifdef __APPLE__
    int main_policy;
    struct sched_param main_param;

    main_policy = SCHED_RR;
    main_param.sched_priority = sched_get_priority_min(SCHED_RR)+2;

    pthread_setschedparam(pthread_self(), main_policy, &main_param);
#endif

    return true;
}

int CubicSDR::OnExit() {
    std::cout << "Terminating SDR thread.." << std::endl;
    sdrThread->terminate();
    t_SDR->join();

    std::cout << "Terminating SDR post-processing thread.." << std::endl;
    sdrPostThread->terminate();
    t_PostSDR->join();

    delete sdrThread;
    delete t_SDR;

    delete sdrPostThread;
    delete t_PostSDR;

    demodMgr.terminateAll();

    delete threadCmdQueueSDR;

    delete iqVisualQueue;
    delete audioVisualQueue;
    delete iqPostDataQueue;

    delete m_glContext;

#ifdef __APPLE__
    AudioThread::deviceCleanup();
#endif

    return wxApp::OnExit();
}

PrimaryGLContext& CubicSDR::GetContext(wxGLCanvas *canvas) {
    PrimaryGLContext *glContext;
    if (!m_glContext) {
        m_glContext = new PrimaryGLContext(canvas, NULL);
    }
    glContext = m_glContext;

    return *glContext;
}

void CubicSDR::setFrequency(long long freq) {
    if (freq < sampleRate / 2) {
        freq = sampleRate / 2;
    }
    frequency = freq;
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_TUNE);
    command.llong_value = freq;
    threadCmdQueueSDR->push(command);
}

long long CubicSDR::getOffset() {
    return offset;
}

void CubicSDR::setOffset(long long ofs) {
    offset = ofs;
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_SET_OFFSET);
    command.llong_value = ofs;
    threadCmdQueueSDR->push(command);
}

long long CubicSDR::getFrequency() {
    return frequency;
}

DemodulatorThreadOutputQueue* CubicSDR::getAudioVisualQueue() {
    return audioVisualQueue;
}

DemodulatorThreadInputQueue* CubicSDR::getIQVisualQueue() {
    return iqVisualQueue;
}

DemodulatorMgr &CubicSDR::getDemodMgr() {
    return demodMgr;
}

void CubicSDR::bindDemodulator(DemodulatorInstance *demod) {
    if (!demod) {
        return;
    }
    sdrPostThread->bindDemodulator(demod);
}

void CubicSDR::setSampleRate(long long rate_in) {
    sampleRate = rate_in;
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_SET_SAMPLERATE);
    command.llong_value = rate_in;
    threadCmdQueueSDR->push(command);
    setFrequency(frequency);
}

long long CubicSDR::getSampleRate() {
    return sampleRate;
}

void CubicSDR::removeDemodulator(DemodulatorInstance *demod) {
    if (!demod) {
        return;
    }
    demod->setActive(false);
    sdrPostThread->removeDemodulator(demod);
}

std::vector<SDRDeviceInfo*>* CubicSDR::getDevices() {
    return &devs;
}

void CubicSDR::setDevice(int deviceId) {
    sdrThread->setDeviceId(deviceId);
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_SET_DEVICE);
    command.llong_value = deviceId;
    threadCmdQueueSDR->push(command);

    SDRDeviceInfo *dev = (*getDevices())[deviceId];

    SDRThreadCommand command_ppm(SDRThreadCommand::SDR_THREAD_CMD_SET_PPM);
    ppm = config.getDevice(dev->getDeviceId())->getPPM();
    command_ppm.llong_value = ppm;
    threadCmdQueueSDR->push(command_ppm);
}

int CubicSDR::getDevice() {
    return sdrThread->getDeviceId();
}

AppConfig *CubicSDR::getConfig() {
    return &config;
}

void CubicSDR::saveConfig() {
    config.save();
}

void CubicSDR::setPPM(int ppm_in) {
    if (sdrThread->getDeviceId() < 0) {
        return;
    }
    ppm = ppm_in;

    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_SET_PPM);
    command.llong_value = ppm;
    threadCmdQueueSDR->push(command);

    SDRDeviceInfo *dev = (*getDevices())[getDevice()];

    config.getDevice(dev->getDeviceId())->setPPM(ppm_in);
    config.save();
}

int CubicSDR::getPPM() {
    if (sdrThread->getDeviceId() < 0) {
        return 0;
    }
    SDRDeviceInfo *dev = (*getDevices())[getDevice()];

    SDRThreadCommand command_ppm(SDRThreadCommand::SDR_THREAD_CMD_SET_PPM);
    ppm = config.getDevice(dev->getDeviceId())->getPPM();

    return ppm;
}


void CubicSDR::showFrequencyInput() {
    FrequencyDialog fdialog(appframe, -1, demodMgr.getActiveDemodulator()?_("Set Demodulator Frequency"):_("Set Frequency"), demodMgr.getActiveDemodulator(), wxPoint(-100,-100), wxSize(320, 75 ));
    fdialog.ShowModal();
}

