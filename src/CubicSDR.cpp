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

#ifdef _OSX_APP_
#include "CoreFoundation/CoreFoundation.h"
#endif

IMPLEMENT_APP(CubicSDR)

CubicSDR::CubicSDR() : appframe(NULL), m_glContext(NULL), frequency(0), offset(0), ppm(0), snap(1), sampleRate(DEFAULT_SAMPLE_RATE), directSamplingMode(0),
    sdrThread(NULL), sdrPostThread(NULL), spectrumVisualThread(NULL), demodVisualThread(NULL), pipeSDRIQData(NULL), pipeIQVisualData(NULL), pipeAudioVisualData(NULL), t_SDR(NULL), t_PostSDR(NULL) {
    
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
    devicesReady.store(false);
    deviceSelectorOpen.store(false);

    // Visual Data
    spectrumVisualThread = new SpectrumVisualDataThread();
    demodVisualThread = new SpectrumVisualDataThread();
    
    pipeIQVisualData = new DemodulatorThreadInputQueue();
    pipeIQVisualData->set_max_num_items(1);

    spectrumDistributor.setInput(pipeIQVisualData);
    
    pipeDemodIQVisualData = new DemodulatorThreadInputQueue();
    pipeDemodIQVisualData->set_max_num_items(1);
    
    pipeSpectrumIQVisualData = new DemodulatorThreadInputQueue();
    pipeSpectrumIQVisualData->set_max_num_items(1);
    
    pipeWaterfallIQVisualData = new DemodulatorThreadInputQueue();
    pipeWaterfallIQVisualData->set_max_num_items(128);
    
    spectrumDistributor.attachOutput(pipeDemodIQVisualData);
    spectrumDistributor.attachOutput(pipeSpectrumIQVisualData);
    
    getDemodSpectrumProcessor()->setInput(pipeDemodIQVisualData);
    getSpectrumProcessor()->setInput(pipeSpectrumIQVisualData);
    
    pipeAudioVisualData = new DemodulatorThreadOutputQueue();
    pipeAudioVisualData->set_max_num_items(1);
    
    scopeProcessor.setInput(pipeAudioVisualData);
    
    // I/Q Data
    pipeSDRIQData = new SDRThreadIQDataQueue();
    pipeSDRIQData->set_max_num_items(100);
    
    sdrThread = new SDRThread();
    sdrThread->setOutputQueue("IQDataOutput",pipeSDRIQData);

    sdrPostThread = new SDRPostThread();
//    sdrPostThread->setNumVisSamples(BUF_SIZE);
    sdrPostThread->setInputQueue("IQDataInput", pipeSDRIQData);
    sdrPostThread->setOutputQueue("IQVisualDataOutput", pipeIQVisualData);
    sdrPostThread->setOutputQueue("IQDataOutput", pipeWaterfallIQVisualData);
    
    t_PostSDR = new std::thread(&SDRPostThread::threadMain, sdrPostThread);
    t_SpectrumVisual = new std::thread(&SpectrumVisualDataThread::threadMain, spectrumVisualThread);
    t_DemodVisual = new std::thread(&SpectrumVisualDataThread::threadMain, demodVisualThread);

//    t_SDR = new std::thread(&SDRThread::threadMain, sdrThread);
    sdrEnum = new SDREnumerator();


    appframe = new AppFrame();
	t_SDREnum = new std::thread(&SDREnumerator::threadMain, sdrEnum);

//#ifdef __APPLE__
//    int main_policy;
//    struct sched_param main_param;
//
//    main_policy = SCHED_RR;
//    main_param.sched_priority = sched_get_priority_min(SCHED_RR)+2;
//
//    pthread_setschedparam(pthread_self(), main_policy, &main_param);
//#endif

    return true;
}

int CubicSDR::OnExit() {
    demodMgr.terminateAll();
    
    std::cout << "Terminating SDR thread.." << std::endl;
    if (!sdrThread->isTerminated()) {
        sdrThread->terminate();
        if (t_SDR) {
            t_SDR->join();
        }
    }
    std::cout << "Terminating SDR post-processing thread.." << std::endl;
    sdrPostThread->terminate();
    t_PostSDR->join();
    
    std::cout << "Terminating Visual Processor threads.." << std::endl;
    spectrumVisualThread->terminate();
    t_SpectrumVisual->join();

    demodVisualThread->terminate();
    t_DemodVisual->join();

    delete sdrThread;

    delete sdrPostThread;
    delete t_PostSDR;

    delete t_SpectrumVisual;
    delete spectrumVisualThread;
    delete t_DemodVisual;
    delete demodVisualThread;
    
    delete pipeIQVisualData;
    delete pipeAudioVisualData;
    delete pipeSDRIQData;

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

void CubicSDR::deviceSelector() {
    if (deviceSelectorOpen) {
        deviceSelectorDialog->Raise();
        deviceSelectorDialog->SetFocus();
        return;
    }
    deviceSelectorOpen.store(true);
    deviceSelectorDialog = new SDRDevicesDialog(appframe);
    deviceSelectorDialog->Show();
}

void CubicSDR::addRemote(std::string remoteAddr) {
    SDREnumerator::addRemote(remoteAddr);
    devicesReady.store(false);
    t_SDREnum = new std::thread(&SDREnumerator::threadMain, sdrEnum);
}

void CubicSDR::removeRemote(std::string remoteAddr) {
    SDREnumerator::removeRemote(remoteAddr);
}

void CubicSDR::sdrThreadNotify(SDRThread::SDRThreadState state, std::string message) {
    notify_busy.lock();
    if (state == SDRThread::SDR_THREAD_MESSAGE) {
        notifyMessage = message;
    }
    if (state == SDRThread::SDR_THREAD_TERMINATED) {
        t_SDR->join();
        delete t_SDR;
    }
    if (state == SDRThread::SDR_THREAD_FAILED) {
        notifyMessage = message;
//        wxMessageDialog *info;
//        info = new wxMessageDialog(NULL, message, wxT("Error initializing device"), wxOK | wxICON_ERROR);
//        info->ShowModal();
    }
    //if (appframe) { appframe->SetStatusText(message); }
    notify_busy.unlock();
}


void CubicSDR::sdrEnumThreadNotify(SDREnumerator::SDREnumState state, std::string message) {
    notify_busy.lock();
    if (state == SDREnumerator::SDR_ENUM_MESSAGE) {
        notifyMessage = message;
    }
    if (state == SDREnumerator::SDR_ENUM_DEVICES_READY) {
        devs = SDREnumerator::enumerate_devices("", true);
        devicesReady.store(true);
    }
    if (state == SDREnumerator::SDR_ENUM_FAILED) {
        notifyMessage = message;
        sdrEnum->terminate();
    }
    //if (appframe) { appframe->SetStatusText(message); }
    notify_busy.unlock();
}


void CubicSDR::setFrequency(long long freq) {
    if (freq < sampleRate / 2) {
        freq = sampleRate / 2;
    }
    frequency = freq;
    sdrThread->setFrequency(freq);
}

long long CubicSDR::getOffset() {
    return offset;
}

void CubicSDR::setOffset(long long ofs) {
    offset = ofs;
    sdrThread->setOffset(offset);
    SDRDeviceInfo *dev = getDevice();
    config.getDevice(dev->getDeviceId())->setOffset(ofs);
}

void CubicSDR::setDirectSampling(int mode) {
    directSamplingMode = mode;
    sdrThread->setDirectSampling(mode);

    SDRDeviceInfo *dev = getDevice();
    config.getDevice(dev->getDeviceId())->setDirectSampling(mode);
}

int CubicSDR::getDirectSampling() {
    return directSamplingMode;
}

void CubicSDR::setSwapIQ(bool swapIQ) {
    sdrPostThread->setSwapIQ(swapIQ);
    SDRDeviceInfo *dev = getDevice();
    config.getDevice(dev->getDeviceId())->setIQSwap(swapIQ);
}

bool CubicSDR::getSwapIQ() {
    return sdrPostThread->getSwapIQ();
}

long long CubicSDR::getFrequency() {
    return frequency;
}

void CubicSDR::setSampleRate(long long rate_in) {
    sampleRate = rate_in;
    sdrThread->setSampleRate(sampleRate);
    setFrequency(frequency);
}

void CubicSDR::setDevice(SDRDeviceInfo *dev) {
    if (!sdrThread->isTerminated()) {
        sdrThread->terminate();
        if (t_SDR) {
            t_SDR->join();
            delete t_SDR;
        }
    }
    
    sdrThread->setDevice(dev);
    
    DeviceConfig *devConfig = config.getDevice(dev->getDeviceId());
    
    SDRDeviceChannel *chan = dev->getRxChannel();
    
    if (chan) {
        long long freqHigh, freqLow;
        
        freqHigh = chan->getRFRange().getHigh();
        freqLow = chan->getRFRange().getLow();
        
// upconverter settings don't like this, need to handle elsewhere..
//        if (frequency > freqHigh) {
//            frequency = freqHigh;
//        }
//        else if (frequency < freqLow) {
//            frequency = freqLow;
//        }
        
        int rateHigh, rateLow;
        rateLow = chan->getSampleRates()[0];
        rateHigh = chan->getSampleRates()[chan->getSampleRates().size()-1];

        if (sampleRate > rateHigh) {
            sampleRate = rateHigh;
        } else if (sampleRate < rateLow) {
            sampleRate = rateLow;
        }
        
        if (frequency < sampleRate/2) {
            frequency = sampleRate/2;
        }
        
        setFrequency(frequency);
        setSampleRate(sampleRate);

        setPPM(devConfig->getPPM());
        setDirectSampling(devConfig->getDirectSampling());
        setSwapIQ(devConfig->getIQSwap());
        setOffset(devConfig->getOffset());
        
        t_SDR = new std::thread(&SDRThread::threadMain, sdrThread);
    }
}

SDRDeviceInfo *CubicSDR::getDevice() {
    return sdrThread->getDevice();
}

ScopeVisualProcessor *CubicSDR::getScopeProcessor() {
    return &scopeProcessor;
}

SpectrumVisualProcessor *CubicSDR::getSpectrumProcessor() {
    return spectrumVisualThread->getProcessor();
}

SpectrumVisualProcessor *CubicSDR::getDemodSpectrumProcessor() {
    return demodVisualThread->getProcessor();
}

VisualDataDistributor<DemodulatorThreadIQData> *CubicSDR::getSpectrumDistributor() {
    return &spectrumDistributor;
}


DemodulatorThreadOutputQueue* CubicSDR::getAudioVisualQueue() {
    return pipeAudioVisualData;
}

DemodulatorThreadInputQueue* CubicSDR::getIQVisualQueue() {
    return pipeIQVisualData;
}

DemodulatorThreadInputQueue* CubicSDR::getWaterfallVisualQueue() {
    return pipeWaterfallIQVisualData;
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
    return devs;
}


AppConfig *CubicSDR::getConfig() {
    return &config;
}

void CubicSDR::saveConfig() {
    config.save();
}

void CubicSDR::setPPM(int ppm_in) {
    ppm = ppm_in;
    sdrThread->setPPM(ppm);

    SDRDeviceInfo *dev = getDevice();
    if (dev) {
        config.getDevice(dev->getDeviceId())->setPPM(ppm_in);
    }
}

int CubicSDR::getPPM() {
    SDRDeviceInfo *dev = sdrThread->getDevice();
    if (dev) {
        ppm = config.getDevice(dev->getDeviceId())->getPPM();
    }
    return ppm;
}


void CubicSDR::showFrequencyInput(FrequencyDialog::FrequencyDialogTarget targetMode) {
    const wxString demodTitle("Set Demodulator Frequency");
    const wxString freqTitle("Set Center Frequency");
    const wxString bwTitle("Set Demodulator Bandwidth");

    wxString title;
    
    switch (targetMode) {
        case FrequencyDialog::FDIALOG_TARGET_DEFAULT:
            title = demodMgr.getActiveDemodulator()?demodTitle:freqTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_BANDWIDTH:
            title = bwTitle;
            break;
        default:
            break;
    }
    
    FrequencyDialog fdialog(appframe, -1, title, demodMgr.getActiveDemodulator(), wxPoint(-100,-100), wxSize(320, 75 ), wxDEFAULT_DIALOG_STYLE, targetMode);
    fdialog.ShowModal();
}

AppFrame *CubicSDR::getAppFrame() {
    return appframe;
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

bool CubicSDR::areDevicesReady() {
    return devicesReady.load();
}

bool CubicSDR::areDevicesEnumerating() {
    return !sdrEnum->isTerminated();
}

std::string CubicSDR::getNotification() {
    std::string msg;
    notify_busy.lock();
    msg = notifyMessage;
    notify_busy.unlock();
    return msg;
}

void CubicSDR::setDeviceSelectorClosed() {
    deviceSelectorOpen.store(false);
}

bool CubicSDR::isDeviceSelectorOpen() {
	return deviceSelectorOpen.load();
}
