// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/statbmp.h>

#include "PrimaryGLContext.h"

#include "ScopeCanvas.h"
#include "SpectrumCanvas.h"
#include "WaterfallCanvas.h"
#include "MeterCanvas.h"
#include "TuningCanvas.h"
#include "ModeSelectorCanvas.h"
#include "GainCanvas.h"
#include "FFTVisualDataThread.h"
#include "SDRDeviceInfo.h"
#include "ModemProperties.h"
//#include "UITestCanvas.h"
#include "FrequencyDialog.h"
#include "BookmarkView.h"
#include "AboutDialog.h"

#include <map>

#define wxID_RT_AUDIO_DEVICE 1000
#define wxID_SET_FREQ_OFFSET 2001
#define wxID_RESET 2002
#define wxID_SET_PPM 2003
#define wxID_SET_TIPS 2004
#define wxID_SET_IQSWAP 2005
#define wxID_SDR_DEVICES 2008
#define wxID_AGC_CONTROL 2009
#define wxID_SDR_START_STOP 2010
#define wxID_LOW_PERF 2011
#define wxID_SET_DB_OFFSET 2012
#define wxID_ABOUT_CUBICSDR 2013

#define wxID_MAIN_SPLITTER 2050
#define wxID_VIS_SPLITTER 2051
#define wxID_BM_SPLITTER 2052

#define wxID_THEME_DEFAULT 2100
#define wxID_THEME_SHARP 2101
#define wxID_THEME_BW 2102
#define wxID_THEME_RAD 2103
#define wxID_THEME_TOUCH 2104
#define wxID_THEME_HD 2105
#define wxID_THEME_RADAR 2106

#define wxID_DISPLAY_BOOKMARKS 2107

#define wxID_BANDWIDTH_BASE 2150
#define wxID_BANDWIDTH_MANUAL_DIALOG 2199
#define wxID_BANDWIDTH_MANUAL 2200

#define wxID_DISPLAY_BASE 2250

#define wxID_SETTINGS_BASE 2300

#define wxID_DEVICE_ID 3500

#define wxID_AUDIO_BANDWIDTH_BASE 9000
#define wxID_AUDIO_DEVICE_MULTIPLIER 50

#ifdef USE_HAMLIB
#define wxID_RIG_TOGGLE 11900
#define wxID_RIG_PORT 11901
#define wxID_RIG_SDR_IF 11902
#define wxID_RIG_CONTROL 11903
#define wxID_RIG_FOLLOW 11904
#define wxID_RIG_CENTERLOCK 11905
#define wxID_RIG_FOLLOW_MODEM 11906
#define wxID_RIG_SERIAL_BASE 11950
#define wxID_RIG_MODEL_BASE 12000
#endif

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();

    void initDeviceParams(SDRDeviceInfo *devInfo);
    void updateDeviceParams();

    void saveSession(std::string fileName);
    bool loadSession(std::string fileName);

    FFTVisualDataThread *getWaterfallDataThread();

    void notifyUpdateModemProperties();
    void setMainWaterfallFFTSize(int fftSize);
    void setScopeDeviceName(std::string deviceName);

    void gkNudgeLeft(DemodulatorInstance *demod, int snap);
    void gkNudgeRight(DemodulatorInstance *demod, int snap);

    int OnGlobalKeyDown(wxKeyEvent &event);
    int OnGlobalKeyUp(wxKeyEvent &event);
    
    void setWaterfallLinesPerSecond(int lps);
    void setSpectrumAvgSpeed(double avg);
    
    FrequencyDialog::FrequencyDialogTarget getFrequencyDialogTarget();
    void refreshGainUI();
    void setViewState(long long center_freq, int bandwidth);
    void setViewState(long long center_freq);

    long long getViewCenterFreq();
    int getViewBandwidth();
    bool isUserDemodBusy();
    
    BookmarkView *getBookmarkView();
    void disableSave(bool state);

    //call this in case the main UI is not 
    //the origin of device changes / sample rate by operator,
    //and must be notified back to update its UI elements
    //(ex: SDR Devices dialog changing the configuration)
    void notifyDeviceChanged();
    
#ifdef _WIN32
	bool canFocus();
#endif
    //set tooltip to window
    void setStatusText(wxWindow* window, std::string statusText);
    void AppFrame::setStatusText(std::string statusText, int value);
    
private:
    void OnMenu(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnDoubleClickSash(wxSplitterEvent& event);
    void OnUnSplit(wxSplitterEvent& event);
    void OnAboutDialogClose(wxCommandEvent& event);
   
    //actionXXXX manage menu actions, return true if the event has been
    //treated.
    bool actionOnMenuAbout(wxCommandEvent& event);
    bool actionOnMenuReset(wxCommandEvent& event);
    bool actionOnMenuSettings(wxCommandEvent& event);
    bool actionOnMenuAGC(wxCommandEvent& event);
    bool actionOnMenuSampleRate(wxCommandEvent& event);
    bool actionOnMenuAudioSampleRate(wxCommandEvent& event);
    bool actionOnMenuDisplay(wxCommandEvent& event);
    bool actionOnMenuLoadSave(wxCommandEvent& event);
    bool actionOnMenuRig(wxCommandEvent& event);

    ScopeCanvas *scopeCanvas;
    SpectrumCanvas *spectrumCanvas;
    WaterfallCanvas *waterfallCanvas;
    ModeSelectorCanvas *demodModeSelector;
#ifdef ENABLE_DIGITAL_LAB
    ModeSelectorCanvas *demodModeSelectorAdv;
#endif
    SpectrumCanvas *demodSpectrumCanvas;
    WaterfallCanvas *demodWaterfallCanvas;
    MeterCanvas *demodSignalMeter;
    MeterCanvas *demodGainMeter;
    TuningCanvas *demodTuner;
//    UITestCanvas *testCanvas;
    MeterCanvas *spectrumAvgMeter;
    MeterCanvas *waterfallSpeedMeter;
    ModeSelectorCanvas *demodMuteButton, *peakHoldButton, *soloModeButton, *deltaLockButton;
    GainCanvas *gainCanvas;
    wxSizerItem *gainSizerItem, *gainSpacerItem;
    wxSplitterWindow *mainVisSplitter, *mainSplitter, *bookmarkSplitter;
    wxBoxSizer *demodTray;
    BookmarkView *bookmarkView;
    
    DemodulatorInstance *activeDemodulator;

    std::vector<RtAudio::DeviceInfo> devices;
    std::map<int,RtAudio::DeviceInfo> inputDevices;
    std::map<int,RtAudio::DeviceInfo> outputDevices;
    std::map<int, wxMenuItem *> outputDeviceMenuItems;
    std::map<int, wxMenuItem *> sampleRateMenuItems;
    std::map<int, wxMenuItem *> audioSampleRateMenuItems;
    std::map<int, wxMenuItem *> directSamplingMenuItems;
    wxMenuBar *menuBar;
    
    wxMenu *sampleRateMenu = nullptr;
    wxMenu *displayMenu = nullptr;
    wxMenuItem *agcMenuItem = nullptr;
    wxMenuItem *iqSwapMenuItem = nullptr;
    wxMenuItem *lowPerfMenuItem = nullptr;
    wxMenu *settingsMenu = nullptr;
    
    SoapySDR::ArgInfoList settingArgs;
    int settingsIdMax;
    std::vector<long> sampleRates;
    long manualSampleRate = -1;
    
    std::string currentSessionFile;
    
    FFTVisualDataThread *waterfallDataThread;
    
    std::thread *t_FFTData;
    SDRDeviceInfo *devInfo;
    std::atomic_bool deviceChanged;
    
    ModemProperties *modemProps;
    std::atomic_bool modemPropertiesUpdated;
	wxMenuItem *showTipMenuItem;

    bool lowPerfMode;

    wxMenuItem *hideBookmarksItem;
    bool saveDisabled;
    
    AboutDialog *aboutDlg;

    std::string lastToolTip;

#ifdef USE_HAMLIB
    void enableRig();
    void disableRig();
    
    wxMenu *rigMenu;
    wxMenuItem *rigEnableMenuItem;
    wxMenuItem *rigPortMenuItem;
    wxMenuItem *rigControlMenuItem;
    wxMenuItem *rigFollowMenuItem;
    wxMenuItem *rigCenterLockMenuItem;
    wxMenuItem *rigFollowModemMenuItem;
    wxMenuItem *sdrIFMenuItem;
    
    std::map<int, wxMenuItem *> rigSerialMenuItems;
    std::map<int, wxMenuItem *> rigModelMenuItems;
    int rigModel;
    int rigSerialRate;
    long long rigSDRIF;
    std::vector<int> rigSerialRates;
    std::string rigPort;
    int numRigs;
    bool rigInit;
#endif

    wxDECLARE_EVENT_TABLE();
};
