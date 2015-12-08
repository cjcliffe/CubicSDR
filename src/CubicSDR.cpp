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
#include <iomanip>

#ifdef _OSX_APP_
#include "CoreFoundation/CoreFoundation.h"
#endif

IMPLEMENT_APP(CubicSDR)

//#ifdef ENABLE_DIGITAL_LAB
//// console output buffer for windows
//#ifdef _WINDOWS
//class outbuf : public std::streambuf {
//	public:
//	outbuf() {
//		setp(0, 0);
//	}
//	virtual int_type overflow(int_type c = traits_type::eof()) {
//		return fputc(c, stdout) == EOF ? traits_type::eof() : c;
//	}
//};
//#endif
//#endif

#ifdef MINGW_PATCH
	FILE _iob[] = { *stdin, *stdout, *stderr };

	extern "C" FILE * __cdecl __iob_func(void)
	{
		return _iob;
	}

	extern "C" int __cdecl __isnan(double x)
	{
		return _finite(x)?0:1;
	}

	extern "C" int __cdecl __isnanf(float x)
	{
		return _finitef(x)?0:1;
	}
#endif


std::string& filterChars(std::string& s, const std::string& allowed) {
    s.erase(remove_if(s.begin(), s.end(), [&allowed](const char& c) {
        return allowed.find(c) == std::string::npos;
    }), s.end());
    return s;
}

std::string frequencyToStr(long long freq) {
    long double freqTemp;
    
    freqTemp = freq;
    std::string suffix("");
    std::stringstream freqStr;
    
    if (freqTemp >= 1.0e9) {
        freqTemp /= 1.0e9;
        freqStr << std::setprecision(10);
        suffix = std::string("GHz");
    } else if (freqTemp >= 1.0e6) {
        freqTemp /= 1.0e6;
        freqStr << std::setprecision(7);
        suffix = std::string("MHz");
    } else if (freqTemp >= 1.0e3) {
        freqTemp /= 1.0e3;
        freqStr << std::setprecision(4);
        suffix = std::string("KHz");
    }
    
    freqStr << freqTemp;
    freqStr << suffix;
    
    return freqStr.str();
}

long long strToFrequency(std::string freqStr) {
    std::string filterStr = filterChars(freqStr, std::string("0123456789.MKGHmkgh"));
    
    int numLen = filterStr.find_first_not_of("0123456789.");
    
    if (numLen == std::string::npos) {
        numLen = freqStr.length();
    }
    
    std::string numPartStr = freqStr.substr(0, numLen);
    std::string suffixStr = freqStr.substr(numLen);
    
    std::stringstream numPartStream;
    numPartStream.str(numPartStr);
    
    long double freqTemp = 0;
    
    numPartStream >> freqTemp;
    
    if (suffixStr.length()) {
        if (suffixStr.find_first_of("Gg") != std::string::npos) {
            freqTemp *= 1.0e9;
        } else if (suffixStr.find_first_of("Mm") != std::string::npos) {
            freqTemp *= 1.0e6;
        } else if (suffixStr.find_first_of("Kk") != std::string::npos) {
            freqTemp *= 1.0e3;
        } else if (suffixStr.find_first_of("Hh") != std::string::npos) {
            // ...
        }
    } else if (numPartStr.find_first_of(".") != std::string::npos || freqTemp <= 3000) {
        freqTemp *= 1.0e6;
    }
    
    return (long long) freqTemp;
}


CubicSDR::CubicSDR() : appframe(NULL), m_glContext(NULL), frequency(0), offset(0), ppm(0), snap(1), sampleRate(DEFAULT_SAMPLE_RATE),
    sdrThread(NULL), sdrPostThread(NULL), spectrumVisualThread(NULL), demodVisualThread(NULL), pipeSDRIQData(NULL), pipeIQVisualData(NULL), pipeAudioVisualData(NULL), t_SDR(NULL), t_PostSDR(NULL) {
        sampleRateInitialized.store(false);
        agcMode.store(true);
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

//#ifdef ENABLE_DIGITAL_LAB
//	// console output for windows
//	#ifdef _WINDOWS
//	if (AllocConsole()) {
//		freopen("CONOUT$", "w", stdout);
//		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
//	}
//	outbuf ob;
//	std::streambuf *sb = std::cout.rdbuf(&ob);
//	std::cout.rdbuf(sb);
//	#endif
//#endif
    
    wxApp::SetAppName("CubicSDR");

    Modem::addModemFactory(new ModemFM);
    Modem::addModemFactory(new ModemFMStereo);
    Modem::addModemFactory(new ModemAM);
    Modem::addModemFactory(new ModemLSB);
    Modem::addModemFactory(new ModemUSB);
    Modem::addModemFactory(new ModemDSB);
    Modem::addModemFactory(new ModemIQ);

#ifdef ENABLE_DIGITAL_LAB
    Modem::addModemFactory(new ModemAPSK);
    Modem::addModemFactory(new ModemASK);
    Modem::addModemFactory(new ModemBPSK);
    Modem::addModemFactory(new ModemDPSK);
#if ENABLE_LIQUID_EXPERIMENTAL
    Modem::addModemFactory(new ModemFSK);
#endif
    Modem::addModemFactory(new ModemGMSK);
    Modem::addModemFactory(new ModemOOK);
    Modem::addModemFactory(new ModemPSK);
    Modem::addModemFactory(new ModemQAM);
    Modem::addModemFactory(new ModemQPSK);
    Modem::addModemFactory(new ModemSQAM);
    Modem::addModemFactory(new ModemST);
#endif
    
    frequency = wxGetApp().getConfig()->getCenterFreq();
    offset = 0;
    ppm = 0;
    devicesReady.store(false);
    deviceSelectorOpen.store(false);

    // Visual Data
    spectrumVisualThread = new SpectrumVisualDataThread();
    demodVisualThread = new SpectrumVisualDataThread();
    
    pipeIQVisualData = new DemodulatorThreadInputQueue();
    pipeIQVisualData->set_max_num_items(1);

    pipeDemodIQVisualData = new DemodulatorThreadInputQueue();
    pipeDemodIQVisualData->set_max_num_items(1);
    
    pipeWaterfallIQVisualData = new DemodulatorThreadInputQueue();
    pipeWaterfallIQVisualData->set_max_num_items(128);
    
    getDemodSpectrumProcessor()->setInput(pipeDemodIQVisualData);
    getSpectrumProcessor()->setInput(pipeIQVisualData);
    getSpectrumProcessor()->setHideDC(true);
    
    pipeAudioVisualData = new DemodulatorThreadOutputQueue();
    pipeAudioVisualData->set_max_num_items(1);
    
    scopeProcessor.setInput(pipeAudioVisualData);
    
    // I/Q Data
    pipeSDRIQData = new SDRThreadIQDataQueue();
    pipeSDRIQData->set_max_num_items(100);
    
    sdrThread = new SDRThread();
    sdrThread->setOutputQueue("IQDataOutput",pipeSDRIQData);

    sdrPostThread = new SDRPostThread();
    sdrPostThread->setInputQueue("IQDataInput", pipeSDRIQData);
    sdrPostThread->setOutputQueue("IQVisualDataOutput", pipeIQVisualData);
    sdrPostThread->setOutputQueue("IQDataOutput", pipeWaterfallIQVisualData);
    sdrPostThread->setOutputQueue("IQActiveDemodVisualDataOutput", pipeDemodIQVisualData);
    
    t_PostSDR = new std::thread(&SDRPostThread::threadMain, sdrPostThread);
    t_SpectrumVisual = new std::thread(&SpectrumVisualDataThread::threadMain, spectrumVisualThread);
    t_DemodVisual = new std::thread(&SpectrumVisualDataThread::threadMain, demodVisualThread);

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

#ifdef BUNDLE_SOAPY_MODS
    if (parser.Found("b")) {
        useLocalMod.store(false);
    } else {
        useLocalMod.store(true);
    }
#else
    useLocalMod.store(true);
#endif

    wxString *modPath = new wxString;

    if (parser.Found("m",modPath)) {
        if (modPath) {
            modulePath = modPath->ToStdString();
        } else {
            modulePath = "";
        }
    }
    
    return true;
}

void CubicSDR::closeDeviceSelector() {
    if (deviceSelectorOpen) {
        deviceSelectorDialog->Close();
    }
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
    if (state == SDRThread::SDR_THREAD_INITIALIZED) {
        appframe->initDeviceParams(getDevice());
    }
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
    //if (appframe) { appframe->SetStatusText(message); }
    notify_busy.unlock();

    if (state == SDREnumerator::SDR_ENUM_FAILED) {
        wxMessageDialog *info;
        info = new wxMessageDialog(NULL, wxT("\nNo SoapySDR modules were found.\n\nCubicSDR requires at least one SoapySDR device support module to be installed.\n\nPlease visit https://github.com/cjcliffe/CubicSDR/wiki and in the build instructions for your platform read the 'Support Modules' section for more information."), wxT("\x28\u256F\xB0\u25A1\xB0\uFF09\u256F\uFE35\x20\u253B\u2501\u253B"), wxOK | wxICON_ERROR);
        info->ShowModal();
    }
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
    
    for (SoapySDR::Kwargs::const_iterator i = settingArgs.begin(); i != settingArgs.end(); i++) {
        sdrThread->writeSetting(i->first, i->second);
    }
    sdrThread->setStreamArgs(streamArgs);
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
        
        // Try for a reasonable default sample rate.
        if (!sampleRateInitialized.load()) {
            sampleRate = chan->getSampleRateNear(DEFAULT_SAMPLE_RATE);
            sampleRateInitialized.store(true);
        }

        int rateHigh, rateLow;

        rateHigh = rateLow = sampleRate;
        
        if (chan->getSampleRates().size()) {
            rateLow = chan->getSampleRates()[0];
            rateHigh = chan->getSampleRates()[chan->getSampleRates().size()-1];
        }
        
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

SDRPostThread *CubicSDR::getSDRPostThread() {
    return sdrPostThread;
}

SDRThread *CubicSDR::getSDRThread() {
    return sdrThread;
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

void CubicSDR::setAGCMode(bool mode) {
    agcMode.store(mode);
    sdrThread->setAGCMode(mode);
}

bool CubicSDR::getAGCMode() {
    return agcMode.load();
}


void CubicSDR::setGain(std::string name, float gain_in) {
    sdrThread->setGain(name,gain_in);
}

float CubicSDR::getGain(std::string name) {
    return sdrThread->getGain(name);
}

void CubicSDR::setStreamArgs(SoapySDR::Kwargs streamArgs_in) {
    streamArgs = streamArgs_in;
}

void CubicSDR::setDeviceArgs(SoapySDR::Kwargs settingArgs_in) {
    settingArgs = settingArgs_in;
}

bool CubicSDR::getUseLocalMod() {
    return useLocalMod.load();
}

std::string CubicSDR::getModulePath() {
    return modulePath;
}
