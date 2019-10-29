// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/statbmp.h>
#include <wx/tooltip.h>

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


#ifdef USE_HAMLIB
class PortSelectorDialog;
#endif

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();

    void initDeviceParams(SDRDeviceInfo *devInfo);

    FFTVisualDataThread *getWaterfallDataThread();
	WaterfallCanvas *getWaterfallCanvas();
    SpectrumCanvas *getSpectrumCanvas();

    void notifyUpdateModemProperties();
    void setMainWaterfallFFTSize(int fftSize);
    void setScopeDeviceName(std::string deviceName);

    int OnGlobalKeyDown(wxKeyEvent &event);
    int OnGlobalKeyUp(wxKeyEvent &event);

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
	/***
	 * UI Elements
	 */
    ScopeCanvas *scopeCanvas;
    SpectrumCanvas *spectrumCanvas, *demodSpectrumCanvas;
    WaterfallCanvas *waterfallCanvas, *demodWaterfallCanvas;
    TuningCanvas *demodTuner;
    MeterCanvas *demodSignalMeter, *demodGainMeter, *spectrumAvgMeter, *waterfallSpeedMeter;
    ModeSelectorCanvas *demodModeSelector, *demodMuteButton, *peakHoldButton, *soloModeButton, *deltaLockButton;
    GainCanvas *gainCanvas;
    BookmarkView *bookmarkView;

    wxSizerItem *gainSizerItem, *gainSpacerItem;
    wxSplitterWindow *mainVisSplitter, *mainSplitter, *bookmarkSplitter;

    wxBoxSizer *demodTray;

    //Use a raw pointer here to prevent a dangling reference
    DemodulatorInstance* activeDemodulator;

    /***
     * Menus
     */
    wxMenuBar *menuBar = nullptr;

	wxMenu *fileMenu = nullptr;

	wxMenu *settingsMenu = nullptr;
	wxMenuItem *showTipMenuItem;
	wxMenuItem *iqSwapMenuItem = nullptr;
	wxMenuItem *agcMenuItem = nullptr;

	wxMenu *sampleRateMenu = nullptr;

	wxMenu *displayMenu = nullptr;
	wxMenuItem *hideBookmarksItem;

    wxMenu *recordingMenu = nullptr;

	//depending on context, maps the item id to wxMenuItem*,
	//OR the submenu item id to its parent  wxMenuItem*.
	std::map<int, wxMenuItem *> sampleRateMenuItems;
	std::map<int, wxMenuItem *> antennaMenuItems;
	std::map<int, wxMenuItem *> settingsMenuItems;
	std::map<int, wxMenuItem *> performanceMenuItems;
	std::map<int, wxMenuItem *> audioSampleRateMenuItems;
	std::map<int, wxMenuItem *> recordingMenuItems;


	/***
	 * Waterfall Data Thread
	 */
	FFTVisualDataThread *waterfallDataThread;
	std::thread *t_FFTData;


	/***
	 * Active Settings
	 */
	bool saveDisabled = false;

	std::string currentSessionFile;
	std::string currentBookmarkFile;

	SoapySDR::ArgInfoList settingArgs;
    int settingsIdMax;

    std::vector<long> sampleRates;
    long manualSampleRate = -1;

	SDRDeviceInfo *devInfo = nullptr;
	std::atomic_bool deviceChanged;

	ModemProperties *modemProps;
	std::atomic_bool modemPropertiesUpdated;

	std::vector<std::string> antennaNames;
    std::string currentTXantennaName;

    AboutDialog *aboutDlg = nullptr;
    std::string lastToolTip;

#ifdef ENABLE_DIGITAL_LAB
    ModeSelectorCanvas *demodModeSelectorAdv;
#endif


    /***
     * wx Events
     */
    void OnMenu(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnDoubleClickSash(wxSplitterEvent& event);
    void OnUnSplit(wxSplitterEvent& event);
    void OnAboutDialogClose(wxCommandEvent& event);
	void OnNewWindow(wxCommandEvent& event);

    /**
     * Session Management
     */
    void saveSession(std::string fileName);
    bool loadSession(std::string fileName);

	/**
	 * Keyboard handlers
	 */
	void gkNudge(DemodulatorInstancePtr demod, int snap);

	void toggleActiveDemodRecording();
	void toggleAllActiveDemodRecording();

	/**
	 * UI init functions
	 */
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

    void initConfigurationSettings();
    void initMenuBar();
    void initIcon();

    wxMenu *makeFileMenu();
    wxMenu *makeAudioSampleRateMenu();
    wxMenu *makeDisplayMenu();
    wxMenu *makeRecordingMenu();
    void updateRecordingMenu();

	wxString getSettingsLabel(const std::string& settingsName,
							  const std::string& settingsValue,
							  const std::string& settingsSuffix = "");


	/**
	 * Menu Action Handlers
	 */
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
	bool actionOnMenuSDRStartStop(wxCommandEvent &event);
	bool actionOnMenuPerformance(wxCommandEvent &event);
	bool actionOnMenuTips(wxCommandEvent &event);
	bool actionOnMenuIQSwap(wxCommandEvent &event);
	bool actionOnMenuFreqOffset(wxCommandEvent &event);
	bool actionOnMenuDBOffset(wxCommandEvent &event);
	bool actionOnMenuSDRDevices(wxCommandEvent &event);
	bool actionOnMenuSetPPM(wxCommandEvent &event);
	bool actionOnMenuClose(wxCommandEvent &event);


	/**
	 * UI Activity Handlers
	 */
	void handleUpdateDeviceParams();
	void handleTXAntennaChange();
    void handleCurrentModem();
    void handleModeSelector();
    void handleGainMeter();
    void handleDemodWaterfallSpectrum();
    void handleSpectrumWaterfall();
    void handleMuteButton();
    void handleScopeProcessor();
    void handleScopeSpectrumProcessors();
    void handleModemProperties();
    void handlePeakHold();


    /**
     * Hamlib/Rig specific
     */
#ifdef USE_HAMLIB
	wxMenu *rigMenu;
    wxMenuItem *rigEnableMenuItem;
    wxMenuItem *rigPortMenuItem;
    wxMenuItem *rigControlMenuItem;
    wxMenuItem *rigFollowMenuItem;
    wxMenuItem *rigCenterLockMenuItem;
    wxMenuItem *rigFollowModemMenuItem;

    std::map<int, wxMenuItem *> rigSerialMenuItems;
    std::map<int, wxMenuItem *> rigModelMenuItems;
    wxMenu *rigModelMenu;
    int rigModel;
    int rigSerialRate;
    long long rigSDRIF;
    std::vector<int> rigSerialRates;
    std::string rigPort;
    int numRigs;
    PortSelectorDialog *rigPortDialog;

    void enableRig();
    void disableRig();

    wxMenu *makeRigMenu();
    void handleRigMenu();
#endif

    wxDECLARE_EVENT_TABLE();
};



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

#define wxID_THEME_DEFAULT 2070
#define wxID_THEME_DEFAULT_JET 2071
#define wxID_THEME_SHARP 2072
#define wxID_THEME_BW 2073
#define wxID_THEME_RAD 2074
#define wxID_THEME_TOUCH 2075
#define wxID_THEME_HD 2076
#define wxID_THEME_RADAR 2077

#define wxID_DISPLAY_BOOKMARKS 2100

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