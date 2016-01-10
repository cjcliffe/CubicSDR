#pragma once

//WX_GL_CORE_PROFILE 1
//WX_GL_MAJOR_VERSION 3
//WX_GL_MINOR_VERSION 2

#include <thread>

#include "GLExt.h"
#include "PrimaryGLContext.h"

#include "ThreadQueue.h"
#ifdef USE_RTL_SDR
    #include "SDRThread.h"
#else
    #include "SoapySDRThread.h"
    #include "SDREnumerator.h"
#endif
#include "SDRPostThread.h"
#include "AudioThread.h"
#include "DemodulatorMgr.h"
#include "AppConfig.h"
#include "AppFrame.h"
#include "FrequencyDialog.h"

#include "ScopeVisualProcessor.h"
#include "SpectrumVisualProcessor.h"
#include "SpectrumVisualDataThread.h"
#include "SDRDevices.h"
#include "Modem.h"

#include "ModemFM.h"
#include "ModemFMStereo.h"
#include "ModemAM.h"
#include "ModemUSB.h"
#include "ModemLSB.h"
#include "ModemDSB.h"
#include "ModemIQ.h"

#ifdef ENABLE_DIGITAL_LAB
#include "ModemAPSK.h"
#include "ModemASK.h"
#include "ModemBPSK.h"
#include "ModemDPSK.h"
#if ENABLE_LIQUID_EXPERIMENTAL
#include "ModemFSK.h"
#endif
#include "ModemGMSK.h"
#include "ModemOOK.h"
#include "ModemPSK.h"
#include "ModemQAM.h"
#include "ModemQPSK.h"
#include "ModemSQAM.h"
#include "ModemST.h"
#endif

#ifdef USE_HAMLIB
class RigThread;
#endif

#include <wx/cmdline.h>

#define NUM_DEMODULATORS 1

std::string& filterChars(std::string& s, const std::string& allowed);
std::string frequencyToStr(long long freq);
long long strToFrequency(std::string freqStr);

class CubicSDR: public wxApp {
public:
    CubicSDR();

    PrimaryGLContext &GetContext(wxGLCanvas *canvas);

    virtual bool OnInit();
    virtual int OnExit();

    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

    void deviceSelector();
    void sdrThreadNotify(SDRThread::SDRThreadState state, std::string message);
    void sdrEnumThreadNotify(SDREnumerator::SDREnumState state, std::string message);
    
    void setFrequency(long long freq);
    long long getFrequency();
    
    void lockFrequency(long long freq);
    bool isFrequencyLocked();
    void unlockFrequency();

    void setOffset(long long ofs);
    long long getOffset();

    void setSampleRate(long long rate_in);
    long long getSampleRate();

    std::vector<SDRDeviceInfo *> *getDevices();
    void setDevice(SDRDeviceInfo *dev);
    SDRDeviceInfo * getDevice();

    ScopeVisualProcessor *getScopeProcessor();
    SpectrumVisualProcessor *getSpectrumProcessor();
    SpectrumVisualProcessor *getDemodSpectrumProcessor();
    
    DemodulatorThreadOutputQueue* getAudioVisualQueue();
    DemodulatorThreadInputQueue* getIQVisualQueue();
    DemodulatorThreadInputQueue* getWaterfallVisualQueue();
    DemodulatorThreadInputQueue* getActiveDemodVisualQueue();
    DemodulatorMgr &getDemodMgr();

    SDRPostThread *getSDRPostThread();
    SDRThread *getSDRThread();

    void bindDemodulator(DemodulatorInstance *demod);
    void removeDemodulator(DemodulatorInstance *demod);

    void setFrequencySnap(int snap);
    int getFrequencySnap();

    AppConfig *getConfig();
    void saveConfig();

    void setPPM(int ppm_in);
    int getPPM();

    void showFrequencyInput(FrequencyDialog::FrequencyDialogTarget targetMode = FrequencyDialog::FDIALOG_TARGET_DEFAULT);
    AppFrame *getAppFrame();
    
    bool areDevicesReady();
    bool areDevicesEnumerating();
    std::string getNotification();
    
    void addRemote(std::string remoteAddr);
    void removeRemote(std::string remoteAddr);

    void setDeviceSelectorClosed();
	bool isDeviceSelectorOpen();
    void closeDeviceSelector();
    
    void setAGCMode(bool mode);
    bool getAGCMode();

    void setGain(std::string name, float gain_in);
    float getGain(std::string name);

    void setStreamArgs(SoapySDR::Kwargs streamArgs_in);
    void setDeviceArgs(SoapySDR::Kwargs settingArgs_in);

    bool getUseLocalMod();
    std::string getModulePath();
    
#ifdef USE_HAMLIB
    RigThread *getRigThread();
    void initRig(int rigModel, std::string rigPort, int rigSerialRate);
    void stopRig();
    bool rigIsActive();
#endif
    
private:
    AppFrame *appframe;
    AppConfig config;
    PrimaryGLContext *m_glContext;
    std::vector<SDRDeviceInfo *> *devs;

    DemodulatorMgr demodMgr;

    long long frequency;
    long long offset;
    int ppm, snap;
    long long sampleRate;
    std::atomic_bool agcMode;

    SDRThread *sdrThread;
    SDREnumerator *sdrEnum;
    SDRPostThread *sdrPostThread;
    SpectrumVisualDataThread *spectrumVisualThread;
    SpectrumVisualDataThread *demodVisualThread;

    SDRThreadIQDataQueue* pipeSDRIQData;
    DemodulatorThreadInputQueue* pipeIQVisualData;
    DemodulatorThreadOutputQueue* pipeAudioVisualData;
    DemodulatorThreadInputQueue* pipeDemodIQVisualData;
    DemodulatorThreadInputQueue* pipeWaterfallIQVisualData;
    DemodulatorThreadInputQueue* pipeActiveDemodIQVisualData;

    ScopeVisualProcessor scopeProcessor;
    
    SDRDevicesDialog *deviceSelectorDialog;

    SoapySDR::Kwargs streamArgs;
    SoapySDR::Kwargs settingArgs;
    
    std::thread *t_SDR, *t_SDREnum, *t_PostSDR, *t_SpectrumVisual, *t_DemodVisual;
    std::atomic_bool devicesReady;
    std::atomic_bool deviceSelectorOpen;
    std::atomic_bool sampleRateInitialized;
    std::atomic_bool useLocalMod;
    std::string notifyMessage;
    std::string modulePath;
    std::mutex notify_busy;
    std::atomic_bool frequency_locked;
    std::atomic_llong lock_freq;
#ifdef USE_HAMLIB
    RigThread *rigThread;
    std::thread *t_Rig;
#endif
};

#ifdef BUNDLE_SOAPY_MODS
static const wxCmdLineEntryDesc commandLineInfo [] =
{
    { wxCMD_LINE_SWITCH, "h", "help", "Command line parameter help", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_OPTION, "c", "config", "Specify a named configuration to use, i.e. '-c ham'" },
    { wxCMD_LINE_OPTION, "m", "modpath", "Load modules from suppplied path, i.e. '-m ~/SoapyMods/'" },
    { wxCMD_LINE_SWITCH, "b", "bundled", "Use bundled SoapySDR modules first instead of local." },
    { wxCMD_LINE_NONE }
};
#else
static const wxCmdLineEntryDesc commandLineInfo [] =
{
    { wxCMD_LINE_SWITCH, "h", "help", "Command line parameter help", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_OPTION, "c", "config", "Specify a named configuration to use, i.e. '-c ham'" },
    { wxCMD_LINE_OPTION, "m", "modpath", "Load modules from suppplied path, i.e. '-m ~/SoapyMods/'" },
    { wxCMD_LINE_NONE }
};
#endif

DECLARE_APP(CubicSDR)
