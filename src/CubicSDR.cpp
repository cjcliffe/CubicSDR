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

#include "ActionDialog.h"

#include <memory>


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
    std::string suffix;
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
    } else if (numPartStr.find_first_of('.') != std::string::npos || freqTemp <= 3000) {
        freqTemp *= 1.0e6;
    }
    
    return (long long) freqTemp;
}



class ActionDialogBookmarkCatastophe : public ActionDialog {
public:
    ActionDialogBookmarkCatastophe() : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Bookmark Last-Loaded Backup Failure :( :( :(")) {
        m_questionText->SetLabelText(wxT("All attempts to recover bookmarks have failed. \nWould you like to exit without touching any more save files?\nClick OK to exit without saving; or Cancel to continue anyways."));
    }
    
    void doClickOK() override {
        wxGetApp().getAppFrame()->disableSave(true);
        wxGetApp().getAppFrame()->Close(false);
    }
};



class ActionDialogBookmarkBackupLoadFailed : public ActionDialog {
public:
    ActionDialogBookmarkBackupLoadFailed() : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Bookmark Backup Load Failure :( :(")) {
        m_questionText->SetLabelText(wxT("Sorry; unable to load your bookmarks backup file. \nWould you like to attempt to load the last succssfully loaded bookmarks file?"));
    }
    
    void doClickOK() override {
        if (wxGetApp().getBookmarkMgr().hasLastLoad("bookmarks.xml")) {
            if (wxGetApp().getBookmarkMgr().loadFromFile("bookmarks.xml.lastloaded",false)) {
                wxGetApp().getBookmarkMgr().updateBookmarks();
                wxGetApp().getBookmarkMgr().updateActiveList();
            } else {
                ActionDialog::showDialog(new ActionDialogBookmarkCatastophe());
            }
        }
    }
};


class ActionDialogBookmarkLoadFailed : public ActionDialog {
public:
    ActionDialogBookmarkLoadFailed() : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Bookmark Load Failure :(")) {
        m_questionText->SetLabelText(wxT("Sorry; unable to load your bookmarks file. \nWould you like to attempt to load the backup file?"));
    }
    
    void doClickOK() override {
        bool loadOk = false;
        if (wxGetApp().getBookmarkMgr().hasBackup("bookmarks.xml")) {
            loadOk = wxGetApp().getBookmarkMgr().loadFromFile("bookmarks.xml.backup",false);
        }
        if (loadOk) {
            wxGetApp().getBookmarkMgr().updateBookmarks();
            wxGetApp().getBookmarkMgr().updateActiveList();
        } else if (wxGetApp().getBookmarkMgr().hasLastLoad("bookmarks.xml")) {
            ActionDialog::showDialog(new ActionDialogBookmarkBackupLoadFailed());
        } else {
            ActionDialog::showDialog(new ActionDialogBookmarkCatastophe());
        }
    }
};



class ActionDialogRigError : public ActionDialog {
public:
    explicit ActionDialogRigError(const std::string& message) : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Rig Control Error")) {
        m_questionText->SetLabelText(message);
    }

    void doClickOK() override {
    }
};


CubicSDR::CubicSDR() : frequency(0), offset(0), ppm(0), snap(1), sampleRate(DEFAULT_SAMPLE_RATE), agcMode(false)
{
        config.load();

        sampleRateInitialized.store(false);
        agcMode.store(true);
        soloMode.store(false);
        shuttingDown.store(false);
        fdlgTarget = FrequencyDialog::FDIALOG_TARGET_DEFAULT;
        stoppedDev = nullptr;

        //set OpenGL configuration:
        m_glContextAttributes = new wxGLContextAttrs();
        
        wxGLContextAttrs glSettings;
        glSettings.CompatibilityProfile().EndList();

        *m_glContextAttributes = glSettings;
}

void CubicSDR::initAudioDevices() const {
    std::vector<RtAudio::DeviceInfo> devices;
    std::map<int, RtAudio::DeviceInfo> inputDevices, outputDevices;

    AudioThread::enumerateDevices(devices);

    int i = 0;

    for (auto & device : devices) {
        if (device.inputChannels) {
            inputDevices[i] = device;
        }
        if (device.outputChannels) {
            outputDevices[i] = device;
        }
        i++;
    }

    wxGetApp().getDemodMgr().setOutputDevices(outputDevices);
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
    //This tends to hang the application on heavy demod (re)creation.
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
        
    
    wxApp::SetAppName(CUBICSDR_INSTALL_NAME);

#ifdef USE_HAMLIB
    t_Rig = nullptr;
    rigThread = nullptr;
    
    RigThread::enumerate();
#endif

    Modem::addModemFactory(ModemFM::factory, "FM", 200000);
    Modem::addModemFactory(ModemNBFM::factory, "NBFM", 12500);
    Modem::addModemFactory(ModemFMStereo::factory, "FMS", 200000);
    Modem::addModemFactory(ModemAM::factory, "AM", 6000);
    Modem::addModemFactory(ModemCW::factory, "CW", 500);
    Modem::addModemFactory(ModemLSB::factory, "LSB", 5400);
    Modem::addModemFactory(ModemUSB::factory, "USB", 5400);
    Modem::addModemFactory(ModemDSB::factory, "DSB", 5400);
    Modem::addModemFactory(ModemIQ::factory, "I/Q", 48000);

#ifdef ENABLE_DIGITAL_LAB
    Modem::addModemFactory(ModemAPSK::factory, "APSK", 200000);
    Modem::addModemFactory(ModemASK::factory, "ASK", 200000);
    Modem::addModemFactory(ModemBPSK::factory, "BPSK", 200000);
    Modem::addModemFactory(ModemDPSK::factory, "DPSK", 200000);
    Modem::addModemFactory(ModemFSK::factory, "FSK", 19200);
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

    initAudioDevices();

    // Visual Data
    spectrumVisualThread = new SpectrumVisualDataThread();
    
    pipeIQVisualData = std::make_shared<DemodulatorThreadInputQueue>();
    pipeIQVisualData->set_max_num_items(1);
    
    pipeWaterfallIQVisualData = std::make_shared<DemodulatorThreadInputQueue>();
    pipeWaterfallIQVisualData->set_max_num_items(128);
    
    getSpectrumProcessor()->setInput(pipeIQVisualData);
    getSpectrumProcessor()->setHideDC(true);
    
    // I/Q Data
    pipeSDRIQData = std::make_shared<SDRThreadIQDataQueue>();
    pipeSDRIQData->set_max_num_items(100);
    
    sdrThread = new SDRThread();
    sdrThread->setOutputQueue("IQDataOutput",pipeSDRIQData);

    sdrPostThread = new SDRPostThread();
    sdrPostThread->setInputQueue("IQDataInput", pipeSDRIQData);

    sdrPostThread->setOutputQueue("IQVisualDataOutput", pipeIQVisualData);
    sdrPostThread->setOutputQueue("IQDataOutput", pipeWaterfallIQVisualData);
     
#if CUBICSDR_ENABLE_VIEW_SCOPE
    pipeAudioVisualData = std::make_shared<DemodulatorThreadOutputQueue>();
    pipeAudioVisualData->set_max_num_items(1);
    
    scopeProcessor.setInput(pipeAudioVisualData);
#else
    pipeAudioVisualData = nullptr;
#endif
    
#if CUBICSDR_ENABLE_VIEW_DEMOD
    demodVisualThread = new SpectrumVisualDataThread();
    pipeDemodIQVisualData = std::make_shared<DemodulatorThreadInputQueue>();
    pipeDemodIQVisualData->set_max_num_items(1);
    
    if (getDemodSpectrumProcessor()) {
        getDemodSpectrumProcessor()->setInput(pipeDemodIQVisualData);
    }
    sdrPostThread->setOutputQueue("IQActiveDemodVisualDataOutput", pipeDemodIQVisualData);
#else
    demodVisualThread = nullptr;
    pipeDemodIQVisualData = nullptr;
    t_DemodVisual = nullptr;
#endif

    // Now that input/output queue plumbing is completely done, we can
    //safely starts all the threads:
    t_SpectrumVisual = new std::thread(&SpectrumVisualDataThread::threadMain, spectrumVisualThread);

    if (demodVisualThread != nullptr) {
        t_DemodVisual = new std::thread(&SpectrumVisualDataThread::threadMain, demodVisualThread);
    }

    //Start SDRPostThread last.
    t_PostSDR = new std::thread(&SDRPostThread::threadMain, sdrPostThread);
    

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

    if (!wxGetApp().getBookmarkMgr().loadFromFile("bookmarks.xml")) {
        if (wxGetApp().getBookmarkMgr().hasBackup("bookmarks.xml")) {
            ActionDialog::showDialog(new ActionDialogBookmarkLoadFailed());
        } else if (wxGetApp().getBookmarkMgr().hasLastLoad("bookmarks.xml")) {
            ActionDialog::showDialog(new ActionDialogBookmarkBackupLoadFailed());
        } else {
            ActionDialog::showDialog(new ActionDialogBookmarkCatastophe());
        }
    } else {
        getBookmarkMgr().updateActiveList();
        getBookmarkMgr().updateBookmarks();
    }
    
    return true;
}

int CubicSDR::OnExit() {
    shuttingDown.store(true);

#if USE_HAMLIB
    if (rigIsActive()) {
        std::cout << "Terminating Rig thread.."  << std::endl << std::flush;
        stopRig();
    }
#endif

    bool terminationSequenceOK = true;

    //The thread feeding them all should be terminated first, so: 
    std::cout << "Terminating SDR thread.." << std::endl << std::flush ;
    sdrThread->terminate();
    terminationSequenceOK = terminationSequenceOK && sdrThread->isTerminated(3000);

    //in case termination sequence goes wrong, kill App brutally now because it can get stuck. 
    if (!terminationSequenceOK) {
        //no trace here because it could occur if the device is not started.  
        ::exit(11);
    }

    std::cout << "Terminating SDR post-processing thread.." << std::endl << std::flush;
    sdrPostThread->terminate();

    //Wait for termination for sdrPostThread second:: since it is doing
    //mostly blocking push() to the other threads, they must stay alive
    //so that sdrPostThread can complete a processing loop and die.
    terminationSequenceOK = terminationSequenceOK && sdrPostThread->isTerminated(3000);

    //in case termination sequence goes wrong, kill App brutally now because it can get stuck. 
    if (!terminationSequenceOK) {
        std::cout << "Cannot terminate application properly, calling exit() now." << std::endl << std::flush;
        ::exit(12);
    }

    std::cout << "Terminating All Demodulators.." << std::endl << std::flush;
    demodMgr.terminateAll();

    std::cout << "Terminating Visual Processor threads.." << std::endl << std::flush;
    spectrumVisualThread->terminate();
    if (demodVisualThread) {
        demodVisualThread->terminate();
    }
    
    //Wait nicely
    terminationSequenceOK = terminationSequenceOK &&  spectrumVisualThread->isTerminated(1000);

    if (demodVisualThread) {
        terminationSequenceOK = terminationSequenceOK && demodVisualThread->isTerminated(1000);
    }

    //in case termination sequence goes wrong, kill App brutally because it can get stuck. 
    if (!terminationSequenceOK) {
        std::cout << "Cannot terminate application properly, calling exit() now." << std::endl << std::flush;
        ::exit(13);
    }

    //Then join the thread themselves:
    if (t_SDR) {
        t_SDR->join();
    }

    t_PostSDR->join();
    
    if (t_DemodVisual) {
        t_DemodVisual->join();
    }
    
    t_SpectrumVisual->join();

    //Now only we can delete:
    delete t_SDR;
    t_SDR = nullptr;

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

    delete m_glContext;
    m_glContext = nullptr;

    //
    AudioThread::deviceCleanup();

    std::cout << "Application termination complete." << std::endl << std::flush;

    return wxApp::OnExit();
}

PrimaryGLContext& CubicSDR::GetContext(wxGLCanvas *canvas) {
    PrimaryGLContext *glContext;
    if (!m_glContext) {
        m_glContext = new PrimaryGLContext(canvas, nullptr, GetContextAttributes());
    }
    glContext = m_glContext;

    return *glContext;
}

wxGLContextAttrs* CubicSDR::GetContextAttributes() {
   
    return m_glContextAttributes;
}

void CubicSDR::OnInitCmdLine(wxCmdLineParser& parser) {
    parser.SetDesc (commandLineInfo);
    parser.SetSwitchChars (wxT("-"));
}

bool CubicSDR::OnCmdLineParsed(wxCmdLineParser& parser) {
    auto *confName = new wxString;
    if (parser.Found("c",confName)) {
        if (!confName->empty()) {
            config.setConfigName(confName->ToStdString());
        }
    }

#ifdef BUNDLE_SOAPY_MODS
    if (parser.Found("b")) {
        useLocalMod.store(false);
    } else {
        useLocalMod.store(true);
    }
#else
    useLocalMod.store(true);
#endif

    auto *modPath = new wxString;

    if (parser.Found("m",modPath)) {
        if (!modPath->empty()) {
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
    wxRect *winRect = getConfig()->getWindow();
    wxPoint pos(wxDefaultPosition);
    if (winRect != nullptr) {
        pos = wxPoint(winRect->x, winRect->y);
    }
    deviceSelectorDialog = new SDRDevicesDialog(appframe, pos);
    deviceSelectorDialog->Show();
}

void CubicSDR::addRemote(const std::string& remoteAddr) {
    SDREnumerator::addRemote(remoteAddr);
    devicesReady.store(false);
    t_SDREnum = new std::thread(&SDREnumerator::threadMain, sdrEnum);
}

void CubicSDR::removeRemote(const std::string& remoteAddr) {
    SDREnumerator::removeRemote(remoteAddr);
}

void CubicSDR::sdrThreadNotify(SDRThread::SDRThreadState state, const std::string& message) {

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
    if (getDemodSpectrumProcessor()) {
        getDemodSpectrumProcessor()->setPeakHold(getSpectrumProcessor()->getPeakHold());
    }
}

long long CubicSDR::getOffset() {
    return offset;
}

void CubicSDR::setOffset(long long ofs) {
    offset = ofs;
    
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setOffset(offset);
    }
}

void CubicSDR::setAntennaName(const std::string& name) {
    antennaName = name;
     
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setAntenna(antennaName);
    }
}

const std::string& CubicSDR::getAntennaName() {
    return antennaName;
}

void CubicSDR::setChannelizerType(SDRPostThreadChannelizerType chType) {
    if (sdrPostThread && !sdrPostThread->isTerminated()) {
        sdrPostThread->setChannelizerType(chType);
    }
}

SDRPostThreadChannelizerType CubicSDR::getChannelizerType() {

    if (sdrPostThread && !sdrPostThread->isTerminated()) {
        return sdrPostThread->getChannelizerType();
    }

    return SDRPostThreadChannelizerType::SDRPostPFBCH;
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
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->unlockFrequency();
    }
}

void CubicSDR::setSampleRate(long long rate_in) {
    sampleRate = rate_in;
    
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setSampleRate(sampleRate);
    }

    setFrequency(frequency);

    if (rate_in <= CHANNELIZER_RATE_MAX / 8) {
        appframe->setMainWaterfallFFTSize(DEFAULT_FFT_SIZE / 4);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(false);
        spectrumVisualThread->getProcessor()->setHideDC(false);
    } else if (rate_in <= CHANNELIZER_RATE_MAX) {
        appframe->setMainWaterfallFFTSize(DEFAULT_FFT_SIZE / 2);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(false);
        spectrumVisualThread->getProcessor()->setHideDC(false);
    } else if (rate_in > CHANNELIZER_RATE_MAX) {
        appframe->setMainWaterfallFFTSize(DEFAULT_FFT_SIZE);
        appframe->getWaterfallDataThread()->getProcessor()->setHideDC(true);
        spectrumVisualThread->getProcessor()->setHideDC(true);
    }
}

void CubicSDR::stopDevice(bool store, int waitMsForTermination) {
    
    //First we must stop the threads
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
        setAGCMode(devConfig->getAGCMode());
        setAntennaName(devConfig->getAntennaName());

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
    if (demodVisualThread) {
        return demodVisualThread->getProcessor();
    } else {
        return nullptr;
    }
}

DemodulatorThreadOutputQueuePtr CubicSDR::getAudioVisualQueue() {
    return pipeAudioVisualData;
}

DemodulatorThreadInputQueuePtr CubicSDR::getIQVisualQueue() {
    return pipeIQVisualData;
}

DemodulatorThreadInputQueuePtr CubicSDR::getWaterfallVisualQueue() {
    return pipeWaterfallIQVisualData;
}

BookmarkMgr &CubicSDR::getBookmarkMgr() {
    return bookmarkMgr;
}

DemodulatorMgr &CubicSDR::getDemodMgr() {
    return demodMgr;
}

SessionMgr &CubicSDR::getSessionMgr() {
    return sessionMgr;
}

SDRPostThread *CubicSDR::getSDRPostThread() {
    return sdrPostThread;
}

SDRThread *CubicSDR::getSDRThread() {
    return sdrThread;
}


void CubicSDR::notifyDemodulatorsChanged() {
    
    sdrPostThread->notifyDemodulatorsChanged();
}

long long CubicSDR::getSampleRate() {
    return sampleRate;
}

void CubicSDR::removeDemodulator(const DemodulatorInstancePtr& demod) {
    if (!demod) {
        return;
    }
    demod->setActive(false);
    sdrPostThread->notifyDemodulatorsChanged();
    wxGetApp().getAppFrame()->notifyUpdateModemProperties();
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
    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setPPM(ppm);
    }
}

int CubicSDR::getPPM() {
    SDRDeviceInfo *dev = sdrThread->getDevice();
    if (dev) {
        ppm = config.getDevice(dev->getDeviceId())->getPPM();
    }
    return ppm;
}

void CubicSDR::showFrequencyInput(FrequencyDialog::FrequencyDialogTarget targetMode, const wxString& initString) {
    const wxString demodTitle("Set Demodulator Frequency");
    const wxString freqTitle("Set Center Frequency");
    const wxString bwTitle("Modem Bandwidth (150Hz - 500KHz)");
    const wxString lpsTitle("Lines-Per-Second (1-1024)");
    const wxString avgTitle("Average Rate (0.1 - 0.99)");
    const wxString gainTitle("Gain Entry: "+wxGetApp().getActiveGainEntry());

    wxString title;
    auto activeModem = demodMgr.getActiveContextModem();

    switch (targetMode) {
        case FrequencyDialog::FDIALOG_TARGET_DEFAULT:
        case FrequencyDialog::FDIALOG_TARGET_FREQ:
            title = activeModem ?demodTitle:freqTitle;
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
            if (wxGetApp().getActiveGainEntry().empty()) {
                return;
            }
            break;
        default:
            break;
    }

    FrequencyDialog fdialog(appframe, -1, title, activeModem, wxPoint(-100,-100), wxSize(350, 75), wxDEFAULT_DIALOG_STYLE, targetMode, initString);
    fdialog.ShowModal();
}

void CubicSDR::showLabelInput() {

    DemodulatorInstancePtr activeDemod = wxGetApp().getDemodMgr().getActiveContextModem();

    if (activeDemod != nullptr) {

        const wxString demodTitle("Edit Demodulator label");

        DemodLabelDialog labelDialog(appframe, -1, demodTitle, activeDemod, wxPoint(-100, -100), wxSize(500, 75), wxDEFAULT_DIALOG_STYLE);
        labelDialog.ShowModal();
    }
}

AppFrame *CubicSDR::getAppFrame() {
    return appframe;
}

void CubicSDR::setFrequencySnap(int snap_in) {
    if (snap_in > 1000000) {
        snap_in = 1000000;
    }
    this->snap = snap_in;
}

int CubicSDR::getFrequencySnap() {
    return snap;
}

bool CubicSDR::areDevicesReady() {
    return devicesReady.load();
}

void CubicSDR::notifyMainUIOfDeviceChange(bool forceRefreshOfGains) {
    appframe->notifyDeviceChanged();

	if (forceRefreshOfGains) {
		appframe->refreshGainUI();
	}
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

    if (sdrThread && !sdrThread->isTerminated()) {
        sdrThread->setAGCMode(mode);
    }
}

bool CubicSDR::getAGCMode() {
    return agcMode.load();
}


void CubicSDR::setGain(const std::string& name, float gain_in) {
    sdrThread->setGain(name,gain_in);
}

float CubicSDR::getGain(const std::string& name) {
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

bool CubicSDR::isShuttingDown()
{
    return shuttingDown.load();
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

        if (rigThread->getErrorState()) {
            ActionDialog::showDialog(new ActionDialogRigError(rigThread->getErrorMessage()));
        }
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
