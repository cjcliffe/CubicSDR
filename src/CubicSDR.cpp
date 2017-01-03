// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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

#ifdef USE_HAMLIB
#include "RigThread.h"
#endif

IMPLEMENT_APP(CubicSDR)

#include <fstream>
#include <clocale>

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
    
    size_t numLen = filterStr.find_first_not_of("0123456789.");
    
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


CubicSDR::CubicSDR() : frequency(0), offset(0), ppm(0), snap(1), sampleRate(DEFAULT_SAMPLE_RATE),agcMode(false)
       {
        sampleRateInitialized.store(false);
        agcMode.store(true);
        soloMode.store(false);
        fdlgTarget = FrequencyDialog::FDIALOG_TARGET_DEFAULT;
        stoppedDev = nullptr;
}

bool CubicSDR::OnInit() {

    //use the current locale most appropriate to this system,
    //so that character-related functions are likely to handle Unicode
    //better (by default, was "C" locale).
    std::setlocale(LC_ALL, "");

//#ifdef _OSX_APP_
//    CFBundleRef mainBundle = CFBundleGetMainBundle();
//    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
//    char path[PATH_MAX];
//    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
//    {
//        // error!
//    }
//    CFRelease(resourcesURL);
//    chdir(path);
//#endif

    if (!wxApp::OnInit()) {
        return false;
    }

    //Deactivated code to allocate an explicit Console on Windows.
    //This tends to hang the apllication on heavy demod (re)creation.
    //To continue to debug with std::cout traces, simply run CubicSDR in a MINSYS2 compatble shell on Windows:
    //ex: Cygwin shell, Git For Windows Bash shell....
#if (0)
    	if (AllocConsole()) {
    		freopen("CONOUT$", "w", stdout);
    		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
            SetConsoleTitle(L"CubicSDR: stdout");
          
    	}

        //refresh
        ofstream ob;
        std::streambuf *sb = std::cout.rdbuf();
        std::cout.rdbuf(sb);
#endif
        
    
    wxApp::SetAppName("CubicSDR");

#ifdef USE_HAMLIB
    t_Rig = nullptr;
    rigThread = nullptr;
    
    RigThread::enumerate();
#endif

    Modem::addModemFactory(ModemFM::factory, "FM", 200000);
    Modem::addModemFactory(ModemNBFM::factory, "NBFM", 12500);
    Modem::addModemFactory(ModemFMStereo::factory, "FMS", 200000);
    Modem::addModemFactory(ModemAM::factory, "AM", 6000);
    Modem::addModemFactory(ModemLSB::factory, "LSB", 5400);
    Modem::addModemFactory(ModemUSB::factory, "USB", 5400);
    Modem::addModemFactory(ModemDSB::factory, "DSB", 5400);
    Modem::addModemFactory(ModemIQ::factory, "I/Q", 48000);

#ifdef ENABLE_DIGITAL_LAB
    Modem::addModemFactory(ModemAPSK::factory, "APSK", 200000);
    Modem::addModemFactory(ModemASK::factory, "ASK", 200000);
    Modem::addModemFactory(ModemBPSK::factory, "BPSK", 200000);
    Modem::addModemFactory(ModemDPSK::factory, "DPSK", 200000);
#if ENABLE_LIQUID_EXPERIMENTAL
    Modem::addModemFactory(ModemFSK::factory, "FSK", 19200);
#endif
    Modem::addModemFactory(ModemGMSK::factory, "GMSK", 19200);
    Modem::addModemFactory(ModemOOK::factory, "OOK", 200000);
    Modem::addModemFactory(ModemPSK::factory, "PSK", 200000);
    Modem::addModemFactory(ModemQAM::factory, "QAM", 200000);
    Modem::addModemFactory(ModemQPSK::factory, "QPSK", 200000);
    Modem::addModemFactory(ModemSQAM::factory, "SQAM", 200000);
    Modem::addModemFactory(ModemST::factory, "ST", 200000);
#endif
    
    frequency = wxGetApp().getConfig()->getCenterFreq();
    offset = 0;
    ppm = 0;
    devicesReady.store(false);
    devicesFailed.store(false);
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
    
    SDREnumerator::setManuals(config.getManualDevices());

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
#if USE_HAMLIB
    if (rigIsActive()) {
        std::cout << "Terminating Rig thread.." << std::endl;
        stopRig();
    }
#endif

    //The thread feeding them all should be terminated first, so: 
    std::cout << "Terminating SDR thread.." << std::endl;
    sdrThread->terminate();
    sdrThread->isTerminated(3000);
   
    if (t_SDR) {
       t_SDR->join();
       delete t_SDR;
       t_SDR = nullptr;
    }

    std::cout << "Terminating SDR post-processing thread.." << std::endl;
    sdrPostThread->terminate();

    std::cout << "Terminating All Demodulators.." << std::endl;
    demodMgr.terminateAll();
   
    std::cout << "Terminating Visual Processor threads.." << std::endl;
    spectrumVisualThread->terminate();
    demodVisualThread->terminate();

    //Wait nicely
    sdrPostThread->isTerminated(1000);
    spectrumVisualThread->isTerminated(1000);
    demodVisualThread->isTerminated(1000);

    //Then join the thread themselves
    t_PostSDR->join();
    t_DemodVisual->join();
    t_SpectrumVisual->join();

    //Now only we can delete
    delete sdrThread;
    sdrThread = nullptr;

    delete sdrPostThread;
    sdrPostThread = nullptr;

    delete t_PostSDR;
    t_PostSDR = nullptr;

    delete t_SpectrumVisual;
    t_SpectrumVisual = nullptr;

    delete spectrumVisualThread;
    spectrumVisualThread = nullptr;

    delete t_DemodVisual;
    t_DemodVisual = nullptr;

    delete demodVisualThread;
    demodVisualThread = nullptr;
    
    delete pipeIQVisualData;
    pipeIQVisualData = nullptr;

    delete pipeAudioVisualData;
    pipeAudioVisualData = nullptr;

    delete pipeSDRIQData;
    pipeSDRIQData = nullptr;

    delete m_glContext;
    m_glContext = nullptr;

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

    std::lock_guard < std::mutex > lock(notify_busy);

   
    if (state == SDRThread::SDR_THREAD_INITIALIZED) {
        appframe->initDeviceParams(getDevice());
    }
    if (state == SDRThread::SDR_THREAD_MESSAGE) {
        notifyMessage = message;
    }
    if (state == SDRThread::SDR_THREAD_FAILED) {
        notifyMessage = message;
//        wxMessageDialog *info;
//        info = new wxMessageDialog(NULL, message, wxT("Error initializing device"), wxOK | wxICON_ERROR);
//        info->ShowModal();
    }
    //if (appframe) { appframe->SetStatusText(message); }
  
}


void CubicSDR::sdrEnumThreadNotify(SDREnumerator::SDREnumState state, std::string message) {
    std::lock_guard < std::mutex > lock(notify_busy);

    if (state == SDREnumerator::SDR_ENUM_MESSAGE) {
        notifyMessage = message;
    }
    if (state == SDREnumerator::SDR_ENUM_DEVICES_READY) {
        devs = SDREnumerator::enumerate_devices("", true);
        devicesReady.store(true);
    }
    if (state == SDREnumerator::SDR_ENUM_FAILED) {
        devicesFailed.store(true);
    }
    //if (appframe) { appframe->SetStatusText(message); }
   

}


void CubicSDR::setFrequency(long long freq) {
    if (freq < sampleRate / 2) {
        freq = sampleRate / 2;
    }
    frequency = freq;
    sdrThread->setFrequency(freq);
    getSpectrumProcessor()->setPeakHold(getSpectrumProcessor()->getPeakHold());

    //make the peak hold act on the current dmod also, like a zoomed-in version.
    getDemodSpectrumProcessor()->setPeakHold(getSpectrumProcessor()->getPeakHold());
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


void CubicSDR::lockFrequency(long long freq) {
    frequency_locked.store(true);
    lock_freq.store(freq);
    
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->lockFrequency(freq);
    }
}

bool CubicSDR::isFrequencyLocked() {
    return frequency_locked.load();
}

void CubicSDR::unlockFrequency() {
    frequency_locked.store(false);
    sdrThread->unlockFrequency();
}

void CubicSDR::setSampleRate(long long rate_in) {
    sampleRate = rate_in;
    sdrThread->setSampleRate(sampleRate);
    setFrequency(frequency);

    if (rate_in <= CHANNELIZER_RATE_MAX / 8) {
        appframe->setMainWaterfallFFTSize(512);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(false);
        spectrumVisualThread->getProcessor()->setHideDC(false);
    } else if (rate_in <= CHANNELIZER_RATE_MAX) {
        appframe->setMainWaterfallFFTSize(1024);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(false);
        spectrumVisualThread->getProcessor()->setHideDC(false);
    } else if (rate_in > CHANNELIZER_RATE_MAX) {
        appframe->setMainWaterfallFFTSize(2048);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(true);
        spectrumVisualThread->getProcessor()->setHideDC(true);
    }
}

void CubicSDR::stopDevice(bool store, int waitMsForTermination) {
    
    //Firt we must stop the threads
    sdrThread->terminate();
    sdrThread->isTerminated(waitMsForTermination);

    if (t_SDR) {
        t_SDR->join();
        delete t_SDR;
        t_SDR = nullptr;
    }
    
    //Only now we can nullify devices
    if (store) {
        stoppedDev = sdrThread->getDevice();
    }
    else {
        stoppedDev = nullptr;
    }

    sdrThread->setDevice(nullptr);
}

void CubicSDR::reEnumerateDevices() {
    devicesReady.store(false);
    devs = nullptr;
    SDREnumerator::reset();
    t_SDREnum = new std::thread(&SDREnumerator::threadMain, sdrEnum);
}

void CubicSDR::setDevice(SDRDeviceInfo *dev, int waitMsForTermination) {

    sdrThread->terminate();
    sdrThread->isTerminated(waitMsForTermination);
    
    if (t_SDR) {
       t_SDR->join();
       delete t_SDR;
       t_SDR = nullptr;
    }
    
    for (SoapySDR::Kwargs::const_iterator i = settingArgs.begin(); i != settingArgs.end(); i++) {
        sdrThread->writeSetting(i->first, i->second);
    }
    sdrThread->setStreamArgs(streamArgs);
    sdrThread->setDevice(dev);
    
    DeviceConfig *devConfig = config.getDevice(dev->getDeviceId());
    
    SoapySDR::Device *soapyDev = dev->getSoapyDevice();
    
    if (soapyDev) {
        if (long devSampleRate = devConfig->getSampleRate()) {
            sampleRate = dev->getSampleRateNear(SOAPY_SDR_RX, 0, devSampleRate);
            sampleRateInitialized.store(true);
        }
        
        if (!sampleRateInitialized.load()) {
            sampleRate = dev->getSampleRateNear(SOAPY_SDR_RX, 0, DEFAULT_SAMPLE_RATE);
            sampleRateInitialized.store(true);
        } else {
            sampleRate = dev->getSampleRateNear(SOAPY_SDR_RX, 0, sampleRate);
        }

        if (frequency < sampleRate/2) {
            frequency = sampleRate/2;
        }

        setFrequency(frequency);
        setSampleRate(sampleRate);

        setPPM(devConfig->getPPM());
        setOffset(devConfig->getOffset());
        

        if (devConfig->getAGCMode()) {
            setAGCMode(true);
        } else {
            setAGCMode(false);
        }

        t_SDR = new std::thread(&SDRThread::threadMain, sdrThread);
}
    
    stoppedDev = nullptr;
}

SDRDeviceInfo *CubicSDR::getDevice() {
    if (!sdrThread->getDevice() && stoppedDev) {
        return stoppedDev;
    }

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

void CubicSDR::bindDemodulators(std::vector<DemodulatorInstance *> *demods) {
    if (!demods) {
        return;
    }
    sdrPostThread->bindDemodulators(demods);
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

void CubicSDR::showFrequencyInput(FrequencyDialog::FrequencyDialogTarget targetMode, wxString initString) {
    const wxString demodTitle("Set Demodulator Frequency");
    const wxString freqTitle("Set Center Frequency");
    const wxString bwTitle("Modem Bandwidth (150Hz - 500KHz)");
    const wxString lpsTitle("Lines-Per-Second (1-1024)");
    const wxString avgTitle("Average Rate (0.1 - 0.99)");
    const wxString gainTitle("Gain Entry: "+wxGetApp().getActiveGainEntry());

    wxString title;
    
    switch (targetMode) {
        case FrequencyDialog::FDIALOG_TARGET_DEFAULT:
            title = demodMgr.getActiveDemodulator()?demodTitle:freqTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_BANDWIDTH:
            title = bwTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_WATERFALL_LPS:
            title = lpsTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_SPECTRUM_AVG:
            title = avgTitle;
            break;
        case FrequencyDialog::FDIALOG_TARGET_GAIN:
            title = gainTitle;
            if (wxGetApp().getActiveGainEntry() == "") {
                return;
            }
            break;
        default:
            break;
    }
    
    FrequencyDialog fdialog(appframe, -1, title, demodMgr.getActiveDemodulator(), wxPoint(-100,-100), wxSize(350, 75), wxDEFAULT_DIALOG_STYLE, targetMode, initString);
    fdialog.ShowModal();
}

void CubicSDR::showLabelInput() {

    DemodulatorInstance *activeDemod = wxGetApp().getDemodMgr().getActiveDemodulator();

    if (activeDemod != nullptr) {

        const wxString demodTitle("Edit Demodulator label");

        DemodLabelDialog labelDialog(appframe, -1, demodTitle, activeDemod, wxPoint(-100, -100), wxSize(500, 75), wxDEFAULT_DIALOG_STYLE);
        labelDialog.ShowModal();
    }
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

bool CubicSDR::areModulesMissing() {
    return devicesFailed.load();
}

std::string CubicSDR::getNotification() {
    std::string msg;
    std::lock_guard < std::mutex > lock(notify_busy);
    msg = notifyMessage;
   
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

void CubicSDR::setActiveGainEntry(std::string gainName) {
    activeGain = gainName;
}

std::string CubicSDR::getActiveGainEntry() {
    return activeGain;
}

void CubicSDR::setSoloMode(bool solo) {
    soloMode.store(solo);
}

bool CubicSDR::getSoloMode() {
    return soloMode.load();
}

int CubicSDR::FilterEvent(wxEvent& event) {
    if (!appframe) {
        return -1;
    }

    if (event.GetEventType() == wxEVT_KEY_DOWN || event.GetEventType() == wxEVT_CHAR_HOOK) {
		return appframe->OnGlobalKeyDown((wxKeyEvent&)event);
    }
    
    if (event.GetEventType() == wxEVT_KEY_UP || event.GetEventType() == wxEVT_CHAR_HOOK) {
        return appframe->OnGlobalKeyUp((wxKeyEvent&)event);
    }
    
    return -1;  // process normally
}

#ifdef USE_HAMLIB
RigThread *CubicSDR::getRigThread() {
    return rigThread;
}

void CubicSDR::initRig(int rigModel, std::string rigPort, int rigSerialRate) {
    if (rigThread) {

        rigThread->terminate();
        rigThread->isTerminated(1000);
    }

    if (t_Rig && t_Rig->joinable()) {
        t_Rig->join();
    }

    //now we can delete
    if (rigThread) {

        delete rigThread;
        rigThread = nullptr;
    }
    if (t_Rig) {
      
        delete t_Rig;
        t_Rig = nullptr;
    }

    rigThread = new RigThread();
    rigThread->initRig(rigModel, rigPort, rigSerialRate);
    rigThread->setControlMode(wxGetApp().getConfig()->getRigControlMode());
    rigThread->setFollowMode(wxGetApp().getConfig()->getRigFollowMode());
    rigThread->setCenterLock(wxGetApp().getConfig()->getRigCenterLock());
    rigThread->setFollowModem(wxGetApp().getConfig()->getRigFollowModem());

    t_Rig = new std::thread(&RigThread::threadMain, rigThread);
}

void CubicSDR::stopRig() {
    if (!rigThread) {
        return;
    }
    
    if (rigThread) {
        
        rigThread->terminate();
        rigThread->isTerminated(1000);
    }

    if (t_Rig && t_Rig->joinable()) {
        t_Rig->join();   
    }

    //now we can delete
    if (rigThread) {

        delete rigThread;
        rigThread = nullptr;
    }

    if (t_Rig) {
       
        delete t_Rig;
        t_Rig = nullptr;
    }
}

bool CubicSDR::rigIsActive() {
    return (rigThread && !rigThread->isTerminated());
}

#endif
