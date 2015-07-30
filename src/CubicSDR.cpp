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

CubicSDR::CubicSDR() : appframe(NULL), m_glContext(NULL), frequency(0), offset(0), ppm(0), snap(1), sampleRate(DEFAULT_SAMPLE_RATE), directSamplingMode(0),
    sdrThread(NULL), sdrPostThread(NULL), threadCmdQueueSDR(NULL), iqPostDataQueue(NULL), iqVisualQueue(NULL), audioVisualQueue(NULL), t_SDR(NULL), t_PostSDR(NULL) {
    
}


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

    frequency = wxGetApp().getConfig()->getCenterFreq();
    offset = 0;
    ppm = 0;
    directSamplingMode = 0;

    // Visual Data
    iqVisualQueue = new DemodulatorThreadInputQueue();
    iqVisualQueue->set_max_num_items(1);
    
    audioVisualQueue = new DemodulatorThreadOutputQueue();
    audioVisualQueue->set_max_num_items(1);
    
    // I/Q Data
    iqPostDataQueue = new SDRThreadIQDataQueue;
    threadCmdQueueSDR = new SDRThreadCommandQueue();

    sdrThread = new SDRThread();
    sdrThread->setInputQueue("SDRCommandQueue",threadCmdQueueSDR);
    sdrThread->setOutputQueue("IQDataOutput",iqPostDataQueue);

    sdrPostThread = new SDRPostThread();
    sdrPostThread->setNumVisSamples(16384 * 2);
    sdrPostThread->setInputQueue("IQDataInput", iqPostDataQueue);
    sdrPostThread->setOutputQueue("IQVisualDataOut", iqVisualQueue);

    std::vector<SDRDeviceInfo *>::iterator devs_i;

    SDRThread::enumerate_rtl(&devs);
    SDRDeviceInfo *dev = NULL;

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
        if (devId == -1) {  // User chose to cancel
            return false;
        }
        
        dev = devs[devId];

        sdrThread->setDeviceId(devId);
    } else if (devs.size() == 1) {
        dev = devs[0];
    }
    
    if (!dev) {
        wxMessageDialog *info;
        info = new wxMessageDialog(NULL, wxT("\x28\u256F\xB0\u25A1\xB0\uFF09\u256F\uFE35\x20\u253B\u2501\u253B"), wxT("RTL-SDR device not found"), wxOK | wxICON_ERROR);
        info->ShowModal();
        return false;
    }

    t_PostSDR = new std::thread(&SDRPostThread::threadMain, sdrPostThread);
    t_SDR = new std::thread(&SDRThread::threadMain, sdrThread);

    appframe = new AppFrame();
    if (dev != NULL) {
        appframe->initDeviceParams(dev->getDeviceId());
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
        ppm = devConfig->getPPM();
        offset = devConfig->getOffset();
        directSamplingMode = devConfig->getDirectSampling();
    }

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
    demodMgr.terminateAll();
    
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

void CubicSDR::OnInitCmdLine(wxCmdLineParser& parser) {
    parser.SetDesc (commandLineInfo);
    parser.SetSwitchChars (wxT("-"));
}

bool CubicSDR::OnCmdLineParsed(wxCmdLineParser& parser) {
    wxString *confName = new wxString;
    if (parser.Found("c",confName)) {
        if (confName) {
            config.setConfigName(confName->ToStdString());
        }
    }
    
    config.load();

    return true;
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
    
    SDRDeviceInfo *dev = (*getDevices())[getDevice()];
    config.getDevice(dev->getDeviceId())->setOffset(ofs);
}

void CubicSDR::setDirectSampling(int mode) {
    directSamplingMode = mode;
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_SET_DIRECT_SAMPLING);
    command.llong_value = mode;
    threadCmdQueueSDR->push(command);

    SDRDeviceInfo *dev = (*getDevices())[getDevice()];
    config.getDevice(dev->getDeviceId())->setDirectSampling(mode);
}

int CubicSDR::getDirectSampling() {
    return directSamplingMode;
}

void CubicSDR::setSwapIQ(bool swapIQ) {
    sdrPostThread->setSwapIQ(swapIQ);
    SDRDeviceInfo *dev = (*getDevices())[getDevice()];
    config.getDevice(dev->getDeviceId())->setIQSwap(swapIQ);
}

bool CubicSDR::getSwapIQ() {
    return sdrPostThread->getSwapIQ();
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
    DeviceConfig *devConfig = config.getDevice(dev->getDeviceId());

    setPPM(devConfig->getPPM());
    setDirectSampling(devConfig->getDirectSampling());
    setSwapIQ(devConfig->getIQSwap());
    setOffset(devConfig->getOffset());
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
    FrequencyDialog fdialog(appframe, -1, demodMgr.getActiveDemodulator()?_("Set Demodulator Frequency"):_("Set Center Frequency"), demodMgr.getActiveDemodulator(), wxPoint(-100,-100), wxSize(320, 75 ));
    fdialog.ShowModal();
}

void CubicSDR::setFrequencySnap(int snap) {
    if (snap > 1000000) {
        snap = 1000000;
    }
    this->snap = snap;
}

int CubicSDR::getFrequencySnap() {
    return snap;
}
