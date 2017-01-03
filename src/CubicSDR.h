// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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
#include "DemodLabelDialog.h"

#include "ScopeVisualProcessor.h"
#include "SpectrumVisualProcessor.h"
#include "SpectrumVisualDataThread.h"
#include "SDRDevices.h"
#include "Modem.h"

#include "ModemFM.h"
#include "ModemNBFM.h"
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
    void setDevice(SDRDeviceInfo *dev, int waitMsForTermination);
    void stopDevice(bool store, int waitMsForTermination);
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
    void bindDemodulators(std::vector<DemodulatorInstance *> *demods);
    void removeDemodulator(DemodulatorInstance *demod);

    void setFrequencySnap(int snap);
    int getFrequencySnap();

    AppConfig *getConfig();
    void saveConfig();

    void setPPM(int ppm_in);
    int getPPM();

    void showFrequencyInput(FrequencyDialog::FrequencyDialogTarget targetMode = FrequencyDialog::FDIALOG_TARGET_DEFAULT, wxString initString = "");
    void showLabelInput();
    AppFrame *getAppFrame();
    
    bool areDevicesReady();
    bool areDevicesEnumerating();
    bool areModulesMissing();
    std::string getNotification();
    
    void addRemote(std::string remoteAddr);
    void removeRemote(std::string remoteAddr);

    void setDeviceSelectorClosed();
    void reEnumerateDevices();
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
    
    void setActiveGainEntry(std::string gainName);
    std::string getActiveGainEntry();

    void setSoloMode(bool solo);
    bool getSoloMode();
    
#ifdef USE_HAMLIB
    RigThread *getRigThread();
    void initRig(int rigModel, std::string rigPort, int rigSerialRate);
    void stopRig();
    bool rigIsActive();
#endif
    
private:
    int FilterEvent(wxEvent& event);
    
    AppFrame *appframe = nullptr;
    AppConfig config;
    PrimaryGLContext *m_glContext = nullptr;
    std::vector<SDRDeviceInfo *> *devs = nullptr;

    DemodulatorMgr demodMgr;

    std::atomic_llong frequency;
    std::atomic_llong offset;
    std::atomic_int ppm, snap;
    std::atomic_llong sampleRate;
    std::atomic_bool agcMode;

    SDRThread *sdrThread = nullptr;
    SDREnumerator *sdrEnum = nullptr;
    SDRPostThread *sdrPostThread = nullptr;
    SpectrumVisualDataThread *spectrumVisualThread = nullptr;
    SpectrumVisualDataThread *demodVisualThread = nullptr;

    SDRThreadIQDataQueue* pipeSDRIQData = nullptr;
    DemodulatorThreadInputQueue* pipeIQVisualData = nullptr;
    DemodulatorThreadOutputQueue* pipeAudioVisualData = nullptr;
    DemodulatorThreadInputQueue* pipeDemodIQVisualData = nullptr;
    DemodulatorThreadInputQueue* pipeWaterfallIQVisualData = nullptr;
    DemodulatorThreadInputQueue* pipeActiveDemodIQVisualData = nullptr;

    ScopeVisualProcessor scopeProcessor;
    
    SDRDevicesDialog *deviceSelectorDialog = nullptr;

    SoapySDR::Kwargs streamArgs;
    SoapySDR::Kwargs settingArgs;
    
    std::thread *t_SDR = nullptr;
    std::thread *t_SDREnum = nullptr;
    std::thread *t_PostSDR = nullptr;
    std::thread *t_SpectrumVisual = nullptr;
    std::thread *t_DemodVisual = nullptr;
    std::atomic_bool devicesReady;
    std::atomic_bool devicesFailed;
    std::atomic_bool deviceSelectorOpen;
    std::atomic_bool sampleRateInitialized;
    std::atomic_bool useLocalMod;
    std::string notifyMessage;
    std::string modulePath;
    
    std::mutex notify_busy;
    
    std::atomic_bool frequency_locked;
    std::atomic_llong lock_freq;
    FrequencyDialog::FrequencyDialogTarget fdlgTarget;
    std::string activeGain;
    std::atomic_bool soloMode;
    SDRDeviceInfo *stoppedDev;
#ifdef USE_HAMLIB
    RigThread* rigThread = nullptr;
    std::thread *t_Rig = nullptr;
#endif
};

static const wxCmdLineEntryDesc commandLineInfo [] =
{
    { wxCMD_LINE_SWITCH, "h", "help", "Command line parameter help", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_OPTION, "c", "config", "Specify a named configuration to use, i.e. '-c ham'", wxCMD_LINE_VAL_STRING, 0 },
    { wxCMD_LINE_OPTION, "m", "modpath", "Load modules from suppplied path, i.e. '-m ~/SoapyMods/'", wxCMD_LINE_VAL_STRING, 0 },
#ifdef BUNDLE_SOAPY_MODS
    { wxCMD_LINE_SWITCH, "b", "bundled", "Use bundled SoapySDR modules first instead of local.", wxCMD_LINE_VAL_NONE, 0 },
#endif
    { wxCMD_LINE_NONE, nullptr, nullptr, nullptr, wxCMD_LINE_VAL_NONE, 0 }
};

DECLARE_APP(CubicSDR)
