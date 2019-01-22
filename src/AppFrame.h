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
#include "DemodulatorInstance.h"
#include "DemodulatorThread.h"
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
#define wxID_SET_DB_OFFSET 2012
#define wxID_ABOUT_CUBICSDR 2013

#define wxID_OPEN_BOOKMARKS 2020
#define wxID_SAVE_BOOKMARKS 2021
#define wxID_SAVEAS_BOOKMARKS 2022
#define wxID_RESET_BOOKMARKS 2023

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

#define wxID_ANTENNA_CURRENT 2350
#define wxID_ANTENNA_CURRENT_TX 2501
#define wxID_ANTENNAS_BASE 2352

#define wxID_PERF_CURRENT 2400
#define wxID_PERF_BASE 2401

#define wxID_DEVICE_ID 3500

#define  wxID_RECORDING_PATH 8500
#define  wxID_RECORDING_SQUELCH_BASE 8501
#define  wxID_RECORDING_SQUELCH_SILENCE 8502
#define  wxID_RECORDING_SQUELCH_SKIP 8503
#define  wxID_RECORDING_SQUELCH_ALWAYS 8504
#define  wxID_RECORDING_FILE_TIME_LIMIT 8505

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

#ifdef USE_HAMLIB
class PortSelectorDialog;
#endif

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();

    wxMenu *makeFileMenu();
   
	wxMenu *makeRecordingMenu();
	void updateRecordingMenu();

    void initDeviceParams(SDRDeviceInfo *devInfo);
    void updateDeviceParams();

    void saveSession(std::string fileName);
    bool loadSession(std::string fileName);

    FFTVisualDataThread *getWaterfallDataThread();

    void notifyUpdateModemProperties();
    void setMainWaterfallFFTSize(int fftSize);
    void setScopeDeviceName(std::string deviceName);

    void gkNudgeLeft(DemodulatorInstancePtr demod, int snap);
    void gkNudgeRight(DemodulatorInstancePtr demod, int snap);

    int OnGlobalKeyDown(wxKeyEvent &event);
    int OnGlobalKeyUp(wxKeyEvent &event);
    
    void toggleActiveDemodRecording();
    void toggleAllActiveDemodRecording();
    
    void setWaterfallLinesPerSecond(int lps);
    void setSpectrumAvgSpeed(double avg);
    
    FrequencyDialog::FrequencyDialogTarget getFrequencyDialogTarget();
    void refreshGainUI();
    void setViewState(long long center_freq, int bandwidth);
    void setViewState();

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
    void setStatusText(std::string statusText, int value);
    
#ifdef USE_HAMLIB
    void setRigControlPort(std::string portName);
    void dismissRigControlPortDialog();
#endif
    
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
	bool actionOnMenuRecording(wxCommandEvent& event);
    bool actionOnMenuRig(wxCommandEvent& event);

    wxString getSettingsLabel(const std::string& settingsName, 
                              const std::string& settingsValue, 
                              const std::string& settingsSuffix = "");

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
    
    //Use a raw pointer here to prevent a dangling reference
    DemodulatorInstance* activeDemodulator;

    std::map<int, wxMenuItem *> sampleRateMenuItems;
    std::map<int, wxMenuItem *> antennaMenuItems;
    
    //depending on context, maps the item id to wxMenuItem*,
    //OR the submenu item id to its parent  wxMenuItem*.
    std::map<int, wxMenuItem *> settingsMenuItems;

    std::map<int, wxMenuItem *> performanceMenuItems;
    
    std::map<int, wxMenuItem *> audioSampleRateMenuItems;

	//
	std::map<int, wxMenuItem *> recordingMenuItems;

    wxMenuBar *menuBar;
    
    wxMenu *sampleRateMenu = nullptr;
    wxMenu *displayMenu = nullptr;
    wxMenuItem *agcMenuItem = nullptr;
    wxMenuItem *iqSwapMenuItem = nullptr;

    wxMenu *fileMenu = nullptr;
    wxMenu *settingsMenu = nullptr;
	wxMenu *recordingMenu = nullptr;
    
    SoapySDR::ArgInfoList settingArgs;
    int settingsIdMax;
    std::vector<long> sampleRates;
    long manualSampleRate = -1;

    std::vector<std::string> antennaNames;

   std::string currentTXantennaName;
    
    std::string currentSessionFile;
	std::string currentBookmarkFile;
    
    FFTVisualDataThread *waterfallDataThread;
    
    std::thread *t_FFTData;
    SDRDeviceInfo *devInfo;
    std::atomic_bool deviceChanged;
    
    ModemProperties *modemProps;
    std::atomic_bool modemPropertiesUpdated;
	wxMenuItem *showTipMenuItem;

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
    
    std::map<int, wxMenuItem *> rigSerialMenuItems;
    std::map<int, wxMenuItem *> rigModelMenuItems;
    int rigModel;
    int rigSerialRate;
    long long rigSDRIF;
    std::vector<int> rigSerialRates;
    std::string rigPort;
    int numRigs;
    PortSelectorDialog *rigPortDialog;
#endif

    wxDECLARE_EVENT_TABLE();

	ModeSelectorCanvas *makeModemSelectorPanel(wxWindow *parent, const wxGLAttributes &attribList);
	WaterfallCanvas *makeWaterfallCanvas(wxWindow *parent, const wxGLAttributes &attribList);
	SpectrumCanvas *makeDemodSpectrumCanvas(wxWindow *parent, const wxGLAttributes &attribList);
	MeterCanvas *makeSignalMeter(wxWindow *parent, const wxGLAttributes &attribList);
	ModeSelectorCanvas *makeDeltaLockButton(wxWindow *parent, const wxGLAttributes &attribList);
	TuningCanvas *makeModemTuner(wxWindow *parent, const wxGLAttributes &attribList);
	MeterCanvas *makeModemGainMeter(wxWindow *parent, const wxGLAttributes &attribList);
	ModeSelectorCanvas *makeSoloModeButton(wxWindow *parent, const wxGLAttributes &attribList);
	ModeSelectorCanvas *makeModemMuteButton(wxWindow *parent, const wxGLAttributes &attribList);
	ModeSelectorCanvas *makePeakHoldButton(wxWindow *parent, const wxGLAttributes &attribList);
	SpectrumCanvas *makeSpectrumCanvas(wxWindow *parent, const wxGLAttributes &attribList);
	MeterCanvas *makeSpectrumAvgMeter(wxWindow *parent, const wxGLAttributes &attribList);
	WaterfallCanvas *makeWaterfall(wxWindow *parent, const wxGLAttributes &attribList);
	MeterCanvas *makeWaterfallSpeedMeter(wxWindow *parent, const wxGLAttributes &attribList);
    ScopeCanvas *makeScopeCanvas(wxPanel *parent, const wxGLAttributes &attribList);
    ModeSelectorCanvas *makeModemAdvSelectorPanel(wxPanel *parent, const wxGLAttributes &attribList);
    ModemProperties *makeModemProperties(wxPanel *parent);

    wxMenu *makeAudioSampleRateMenu();
    wxMenu *makeDisplayMenu();
#ifdef USE_HAMLIB
    wxMenu *makeRigMenu();
#endif

};
