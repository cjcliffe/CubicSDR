// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AppFrame.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/numdlg.h"
#include "wx/filedlg.h"

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include <vector>
#include <algorithm>
#include "AudioSinkFileThread.h"
#include "CubicSDR.h"
#include "DataTree.h"
#include "ColorTheme.h"
#include "DemodulatorMgr.h"
#include "ImagePanel.h"
#include "ActionDialog.h"

#include <thread>
#include <iostream>
#include <iomanip>

#include <wx/panel.h>
#include <wx/numformatter.h>
#include <stddef.h>

#if defined(__linux__) || defined(__FreeBSD__)
#include "CubicSDR.xpm"
#endif

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame)
//EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_CLOSE(AppFrame::OnClose)
EVT_MENU(wxID_ANY, AppFrame::OnMenu)
EVT_IDLE(AppFrame::OnIdle)
EVT_SPLITTER_DCLICK(wxID_ANY, AppFrame::OnDoubleClickSash)
EVT_SPLITTER_UNSPLIT(wxID_ANY, AppFrame::OnUnSplit)
wxEND_EVENT_TABLE()

#ifdef USE_HAMLIB
#include "RigThread.h"
#include "PortSelectorDialog.h"
#include "rs232.h"
#endif



class ActionDialogBookmarkReset : public ActionDialog {
public:
    ActionDialogBookmarkReset() : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Reset Bookmarks?")) {
        m_questionText->SetLabelText(wxT("Resetting bookmarks will erase all current bookmarks; are you sure?"));
    }
    
    void doClickOK() {
        wxGetApp().getBookmarkMgr().resetBookmarks();
        wxGetApp().getBookmarkMgr().updateBookmarks();
        wxGetApp().getBookmarkMgr().updateActiveList();
    }
};


#define APPFRAME_MODEMPROPS_MINSIZE 20
#define APPFRAME_MODEMPROPS_MAXSIZE 240

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, CUBICSDR_TITLE), activeDemodulator(nullptr) {

    initIcon();

    deviceChanged.store(false);
    modemPropertiesUpdated.store(false);

    demodTray = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *demodScopeTray = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodTunerTray = new wxBoxSizer(wxHORIZONTAL);

    // OpenGL settings:
    //deprecated format: std::vector<int> attribList = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
    wxGLAttributes attribList;
    attribList.PlatformDefaults().RGBA().MinRGBA(8, 8, 8, 8).DoubleBuffer().EndList();

    mainSplitter = new wxSplitterWindow( this, wxID_MAIN_SPLITTER, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE );
    mainSplitter->SetSashGravity(10.0f / 37.0f);
    mainSplitter->SetMinimumPaneSize(1);

    wxPanel *demodPanel = new wxPanel(mainSplitter, wxID_ANY);

#ifdef CUBICSDR_HEADER_IMAGE
    wxFileName exePath = wxFileName(wxStandardPaths::Get().GetExecutablePath());
    std::string headerPath = exePath.GetPath().ToStdString();
    headerPath += filePathSeparator + std::string("" CUBICSDR_HEADER_IMAGE);
    wxInitAllImageHandlers();

    ImagePanel *imgPanel = new ImagePanel(demodPanel, headerPath, wxBITMAP_TYPE_ANY);

    std::string headerBgColor = "" CUBICSDR_HEADER_BG;
    if (headerBgColor != "") {
        imgPanel->SetBackgroundColour(wxColour(headerBgColor));
    }

    imgPanel->SetBestFittingSize(wxSize(200, 0));

    demodTray->Add(imgPanel, 0, wxEXPAND | wxALL, 0);
    demodTray->AddSpacer(1);
#endif
            
    gainCanvas = new GainCanvas(demodPanel, attribList);
    gainCanvas->setHelpTip("Tuner gains, usually in dB. Click / use Mousewheel to change.");
    gainSizerItem = demodTray->Add(gainCanvas, 0, wxEXPAND | wxALL, 0);
    gainSizerItem->Show(false);
    gainSpacerItem = demodTray->AddSpacer(1);
    gainSpacerItem->Show(false);

    // Demod mode selector
    demodModeSelector = makeModemSelectorPanel(demodPanel, attribList);

    demodTray->Add(demodModeSelector, 2, wxEXPAND | wxALL, 0);
    
#ifdef ENABLE_DIGITAL_LAB
    // Digital Lab Mode Selector
    demodModeSelectorAdv = makeModemAdvSelectorPanel(demodPanel, attribList);
    demodTray->Add(demodModeSelectorAdv, 3, wxEXPAND | wxALL, 0);
#endif
            
    // Modem properties panel
    modemProps = makeModemProperties(demodPanel);

    demodTray->Add(modemProps, 15, wxEXPAND | wxALL, 0);

#ifndef __APPLE__
    demodTray->AddSpacer(1);
#endif
      
#if CUBICSDR_ENABLE_VIEW_DEMOD
    // Demodulator View
    wxBoxSizer *demodVisuals = new wxBoxSizer(wxVERTICAL);

    // Demod Spectrum
    demodSpectrumCanvas = makeDemodSpectrumCanvas(demodPanel, attribList);
    demodVisuals->Add(demodSpectrumCanvas, 3, wxEXPAND | wxALL, 0);

    wxGetApp().getDemodSpectrumProcessor()->setup(DEFAULT_DMOD_FFT_SIZE);
    wxGetApp().getDemodSpectrumProcessor()->attachOutput(demodSpectrumCanvas->getVisualDataQueue());

    demodVisuals->AddSpacer(1);

    // Demod Waterfall
    demodWaterfallCanvas = makeWaterfallCanvas(demodPanel, attribList);
    demodWaterfallCanvas->attachSpectrumCanvas(demodSpectrumCanvas);
    demodVisuals->Add(demodWaterfallCanvas, 6, wxEXPAND | wxALL, 0);

    demodSpectrumCanvas->attachWaterfallCanvas(demodWaterfallCanvas);

    wxGetApp().getDemodSpectrumProcessor()->attachOutput(demodWaterfallCanvas->getVisualDataQueue());
    demodVisuals->SetMinSize(wxSize(128,-1));

    demodTray->Add(demodVisuals, 30, wxEXPAND | wxALL, 0);
    demodTray->AddSpacer(1);
#else
    demodSpectrumCanvas = nullptr;
    demodWaterfallCanvas = nullptr;
#endif

    // Demod Signal/Squelch Meter
    demodSignalMeter = makeSignalMeter(demodPanel, attribList);
    demodTray->Add(demodSignalMeter, 1, wxEXPAND | wxALL, 0);
    demodTray->AddSpacer(1);

#if CUBICSDR_ENABLE_VIEW_SCOPE
    // Scope
    scopeCanvas = makeScopeCanvas(demodPanel, attribList);
    demodScopeTray->Add(scopeCanvas, 8, wxEXPAND | wxALL, 0);

    wxGetApp().getScopeProcessor()->setup(DEFAULT_SCOPE_FFT_SIZE);
    wxGetApp().getScopeProcessor()->attachOutput(scopeCanvas->getInputQueue());

    demodScopeTray->AddSpacer(1);
#else
    scopeCanvas = nullptr;
#endif

    // Delta Lock Button
    deltaLockButton = makeDeltaLockButton(demodPanel, attribList);

    demodTunerTray->Add(deltaLockButton, 0, wxEXPAND | wxALL, 0);
    demodTunerTray->AddSpacer(1);

    // Modem Tuner
    demodTuner = makeModemTuner(demodPanel, attribList);
    demodTunerTray->Add(demodTuner, 1, wxEXPAND | wxALL, 0);

    demodScopeTray->Add(demodTunerTray, 1, wxEXPAND | wxALL, 0);
    demodTray->Add(demodScopeTray, 30, wxEXPAND | wxALL, 0);
    demodTray->AddSpacer(1);

    wxBoxSizer *demodGainTray = new wxBoxSizer(wxVERTICAL);

    // Demod Gain Meter
    demodGainMeter = makeModemGainMeter(demodPanel, attribList);
    demodGainTray->Add(demodGainMeter, 8, wxEXPAND | wxALL, 0);
    demodGainTray->AddSpacer(1);

    // Solo Button
    soloModeButton = makeSoloModeButton(demodPanel, attribList);
    demodGainTray->Add(soloModeButton, 1, wxEXPAND | wxALL, 0);
    demodGainTray->AddSpacer(1);

    // Mute Button
    demodMuteButton = makeModemMuteButton(demodPanel, attribList);
    demodGainTray->Add(demodMuteButton, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodGainTray, 1, wxEXPAND | wxALL, 0);
    
    demodPanel->SetSizer(demodTray);

    bookmarkSplitter = new wxSplitterWindow(mainSplitter, wxID_BM_SPLITTER, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE );
    bookmarkSplitter->SetMinimumPaneSize(1);
    bookmarkSplitter->SetSashGravity(1.0f / 20.0f);
        
    mainVisSplitter = new wxSplitterWindow( bookmarkSplitter, wxID_VIS_SPLITTER, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE );
    mainVisSplitter->SetMinimumPaneSize(1);
    mainVisSplitter->SetSashGravity(6.0f / 25.0f);
        
    wxPanel *spectrumPanel = new wxPanel(mainVisSplitter, wxID_ANY);
    wxBoxSizer *spectrumSizer = new wxBoxSizer(wxHORIZONTAL);

    // Spectrum Canvas
    spectrumCanvas = makeSpectrumCanvas(spectrumPanel, attribList);

    wxGetApp().getSpectrumProcessor()->setup(DEFAULT_FFT_SIZE);
    wxGetApp().getSpectrumProcessor()->attachOutput(spectrumCanvas->getVisualDataQueue());

    spectrumSizer->Add(spectrumCanvas, 63, wxEXPAND | wxALL, 0);
    spectrumSizer->AddSpacer(1);

    wxBoxSizer *spectrumCtlTray = new wxBoxSizer(wxVERTICAL);

    // Peak Hold
    peakHoldButton = makePeakHoldButton(spectrumPanel, attribList);
    spectrumCtlTray->Add(peakHoldButton, 1, wxEXPAND | wxALL, 0);
    spectrumCtlTray->AddSpacer(1);

    // Spectrum Average Meter
    spectrumAvgMeter = makeSpectrumAvgMeter(spectrumPanel, attribList);
    spectrumCtlTray->Add(spectrumAvgMeter, 8, wxEXPAND | wxALL, 0);
    spectrumSizer->Add(spectrumCtlTray, 1, wxEXPAND | wxALL, 0);
    spectrumPanel->SetSizer(spectrumSizer);

    wxPanel *waterfallPanel = new wxPanel(mainVisSplitter, wxID_ANY);
    wxBoxSizer *waterfallSizer = new wxBoxSizer(wxHORIZONTAL);

    // Waterfall
    waterfallCanvas = makeWaterfall(waterfallPanel, attribList);
    // Create and connect the FFT visual data thread
    waterfallDataThread = new FFTVisualDataThread();
    waterfallDataThread->setInputQueue("IQDataInput", wxGetApp().getWaterfallVisualQueue());
    waterfallDataThread->setOutputQueue("FFTDataOutput", waterfallCanvas->getVisualDataQueue());
    waterfallDataThread->getProcessor()->setHideDC(true);

    t_FFTData = new std::thread(&FFTVisualDataThread::threadMain, waterfallDataThread);


    // Waterfall speed meter
    waterfallSpeedMeter = makeWaterfallSpeedMeter(waterfallPanel, attribList);

    waterfallSizer->Add(waterfallCanvas, 63, wxEXPAND | wxALL, 0);
    waterfallSizer->AddSpacer(1);
    waterfallSizer->Add(waterfallSpeedMeter, 1, wxEXPAND | wxALL, 0);
    waterfallPanel->SetSizer(waterfallSizer);

    mainVisSplitter->SplitHorizontally( spectrumPanel, waterfallPanel, 0 );

    // Bookmark View
    bookmarkView = new BookmarkView(bookmarkSplitter, wxID_ANY, wxDefaultPosition, wxSize(120,-1));
    bookmarkSplitter->SplitVertically( bookmarkView, mainVisSplitter );
    mainSplitter->SplitHorizontally( demodPanel, bookmarkSplitter );


    // TODO: refactor these..
    waterfallCanvas->attachSpectrumCanvas(spectrumCanvas);
    spectrumCanvas->attachWaterfallCanvas(waterfallCanvas);

    // Primary sizer for the window
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add(mainSplitter, 1, wxEXPAND | wxALL, 0);

/* * /
    vbox->AddSpacer(1);
    testCanvas = new UITestCanvas(this, attribList);
    vbox->Add(testCanvas, 20, wxEXPAND | wxALL, 0);
// */
            
    this->SetSizer(vbox);

    // Load and apply configuration
    initConfigurationSettings();

    // Initialize menu
    initMenuBar();

    // Create status bar
    CreateStatusBar();

    // Show the window
    Show();

    // Force refresh of all
    Refresh();

    // Pop up the device selector
    wxGetApp().deviceSelector();
}

void AppFrame::initIcon() {
#if defined(__linux__) || defined(__FreeBSD__)
    SetIcon(wxICON(cubicsdr));
#endif
#ifdef _WIN32
    SetIcon(wxICON(frame_icon));
#endif

}

void AppFrame::initMenuBar() {// File Menu
    menuBar = new wxMenuBar;
    fileMenu = makeFileMenu();
    menuBar->Append(fileMenu, wxT("&File"));

    // Settings Menu
    settingsMenu = new wxMenu;
    menuBar->Append(settingsMenu, wxT("&Settings"));

    // Sample Rate Menu
    sampleRateMenu = new wxMenu;
    menuBar->Append(sampleRateMenu, wxT("Sample &Rate"));

    // Audio Sample Rate Menu
    menuBar->Append(makeAudioSampleRateMenu(), wxT("Audio &Sample Rate"));

    //Add a Recording menu
    recordingMenu = makeRecordingMenu();
    menuBar->Append(recordingMenu, wxT("Recordin&g"));
    updateRecordingMenu();

#ifdef USE_HAMLIB
    rigPortDialog = nullptr;

    // Rig Menu
    rigMenu = makeRigMenu();
    menuBar->Append(rigMenu, wxT("&Rig Control"));
#endif

    // Display Menu
    displayMenu = makeDisplayMenu();
    menuBar->Append(displayMenu, wxT("&Display"));

    SetMenuBar(menuBar);

    wxAcceleratorEntry entries[3];
    entries[0].Set(wxACCEL_CTRL, (int) 'O', wxID_OPEN);
    entries[1].Set(wxACCEL_CTRL, (int) 'S', wxID_SAVE);
    entries[2].Set(wxACCEL_CTRL, (int) 'A', wxID_SAVEAS);

    wxAcceleratorTable accel(3, entries);
    SetAcceleratorTable(accel);
}

void AppFrame::initConfigurationSettings() {

    // Init Theme
    ThemeMgr::mgr.setTheme(wxGetApp().getConfig()->getTheme());
    bookmarkView->updateTheme();

    // Init Font Scale
    int fontScale = wxGetApp().getConfig()->getFontScale();
    GLFont::setScale((GLFont::GLFontScale)fontScale);

    // Init window position from configuration
    wxRect *win = wxGetApp().getConfig()->getWindow();
    if (win) {
        SetPosition(win->GetPosition());
        SetClientSize(win->GetSize());
    } else {
        SetClientSize(1280, 600);
        Centre();
    }

    // Init maximize state
    if (wxGetApp().getConfig()->getWindowMaximized()) {
        Maximize();
    }

    // Init frequency snap
    wxGetApp().setFrequencySnap(wxGetApp().getConfig()->getSnap());

    // Init spectrum average speed
    float spectrumAvg = wxGetApp().getConfig()->getSpectrumAvgSpeed();
    spectrumAvgMeter->setLevel(spectrumAvg);
    wxGetApp().getSpectrumProcessor()->setFFTAverageRate(spectrumAvg);

    // Init waterfall speed
    int wflps =wxGetApp().getConfig()->getWaterfallLinesPerSec();
    waterfallSpeedMeter->setLevel(sqrtf(wflps));
    waterfallDataThread->setLinesPerSecond(wflps);
    waterfallCanvas->setLinesPerSecond(wflps);

    // Init modem property collapsed state
    int mpc =wxGetApp().getConfig()->getModemPropsCollapsed();
    if (mpc) {
        modemProps->setCollapsed(true);
    }

    // Init main splitter position (separates top/bottom area horizontally)
    int msPos = wxGetApp().getConfig()->getMainSplit();
    if (msPos != -1) {
        mainSplitter->SetSashPosition(msPos);
    }

    // Init bookmark splitter position (separates bookmark/main vis vertically)
    int bsPos = wxGetApp().getConfig()->getBookmarkSplit();
    if (bsPos != -1) {
        bookmarkSplitter->SetSashPosition(bsPos);
    }

    // Init vis splitter position (separates spectrum/waterfall horizontally)
    int vsPos = wxGetApp().getConfig()->getVisSplit();
    if (vsPos != -1) {
        mainVisSplitter->SetSashPosition(vsPos);
    }

    if (!wxGetApp().getConfig()->getBookmarksVisible()) {
        bookmarkSplitter->Unsplit(bookmarkView);
        bookmarkSplitter->Layout();
    }

#ifdef USE_HAMLIB
    rigModel = wxGetApp().getConfig()->getRigModel();
    rigSerialRate = wxGetApp().getConfig()->getRigRate();
    rigPort = wxGetApp().getConfig()->getRigPort();
#endif
}

ModemProperties *AppFrame::makeModemProperties(wxPanel *parent) {
    ModemProperties *pProperties = new ModemProperties(parent, wxID_ANY);
    pProperties->SetMinSize(wxSize(APPFRAME_MODEMPROPS_MAXSIZE, -1));
    pProperties->SetMaxSize(wxSize(APPFRAME_MODEMPROPS_MAXSIZE, -1));

    ModemArgInfoList dummyInfo;
    pProperties->initProperties(dummyInfo, nullptr);
    pProperties->updateTheme();
    return pProperties;
}

ModeSelectorCanvas *AppFrame::makeModemAdvSelectorPanel(wxPanel *parent, const wxGLAttributes &attribList) {
    ModeSelectorCanvas *pCanvas = new ModeSelectorCanvas(parent, attribList);
    pCanvas->addChoice("ASK");
    pCanvas->addChoice("APSK");
    pCanvas->addChoice("BPSK");
    pCanvas->addChoice("DPSK");
    pCanvas->addChoice("PSK");
    pCanvas->addChoice("FSK");
    pCanvas->addChoice("GMSK");
    pCanvas->addChoice("OOK");
    pCanvas->addChoice("ST");
    pCanvas->addChoice("SQAM");
    pCanvas->addChoice("QAM");
    pCanvas->addChoice("QPSK");
    pCanvas->setHelpTip("Choose advanced modulation types.");
    pCanvas->SetMinSize(wxSize(50, -1));
    pCanvas->SetMaxSize(wxSize(50, -1));
    return pCanvas;
}

#ifdef USE_HAMLIB
wxMenu *AppFrame::makeRigMenu() {
    auto *pMenu = new wxMenu;

    rigEnableMenuItem = pMenu->AppendCheckItem(wxID_RIG_TOGGLE, wxT("Enable Rig"));

    pMenu->Append(wxID_RIG_SDR_IF, wxT("SDR-IF"));

    rigControlMenuItem = pMenu->AppendCheckItem(wxID_RIG_CONTROL, wxT("Control Rig"));
    rigControlMenuItem->Check(wxGetApp().getConfig()->getRigControlMode());

    rigFollowMenuItem = pMenu->AppendCheckItem(wxID_RIG_FOLLOW, wxT("Follow Rig"));
    rigFollowMenuItem->Check(wxGetApp().getConfig()->getRigFollowMode());

    rigCenterLockMenuItem = pMenu->AppendCheckItem(wxID_RIG_CENTERLOCK, wxT("Floating Center"));
    rigCenterLockMenuItem->Check(wxGetApp().getConfig()->getRigCenterLock());

    rigFollowModemMenuItem = pMenu->AppendCheckItem(wxID_RIG_FOLLOW_MODEM, wxT("Track Modem"));
    rigFollowModemMenuItem->Check(wxGetApp().getConfig()->getRigFollowModem());

    auto *rigModelMenu = new wxMenu;
    RigList &rl = RigThread::enumerate();

    std::map<string, int> mfgCount;
    std::map<string, wxMenu *> mfgMenu;
    for (auto ri : rl) {
        mfgCount[ri->mfg_name]++;
    }

    int modelMenuId = wxID_RIG_MODEL_BASE;

    for (auto ri : rl) {
        string modelString(ri->mfg_name);
        modelString.append(" ");
        modelString.append(ri->model_name);

        wxMenu *parentMenu = nullptr;

        if (mfgCount[ri->mfg_name] > 1) {
            if (mfgMenu.find(ri->mfg_name) == mfgMenu.end()) {
                rigModelMenu->AppendSubMenu(mfgMenu[ri->mfg_name] = new wxMenu(), ri->mfg_name);
            }
            parentMenu = mfgMenu[ri->mfg_name];
        } else {
            parentMenu = rigModelMenu;
        }

        rigModelMenuItems[ri->rig_model] = parentMenu->AppendCheckItem(modelMenuId, modelString, ri->copyright);

        if (rigModel == ri->rig_model) {
            rigModelMenuItems[ri->rig_model]->Check(true);
        }

        modelMenuId++;
    }

    pMenu->AppendSubMenu(rigModelMenu, wxT("Model"));

    auto *rigSerialMenu = new wxMenu;

    rigSerialRates.push_back(1200);
    rigSerialRates.push_back(2400);
    rigSerialRates.push_back(4800);
    rigSerialRates.push_back(9600);
    rigSerialRates.push_back(19200);
    rigSerialRates.push_back(38400);
    rigSerialRates.push_back(57600);
    rigSerialRates.push_back(115200);
    rigSerialRates.push_back(128000);
    rigSerialRates.push_back(256000);

    int rateMenuId = wxID_RIG_SERIAL_BASE;
    for (auto rate_i : rigSerialRates) {
        string rateString;
        rateString.append(std::to_string(rate_i));
        rateString.append(" baud");

        rigSerialMenuItems[rate_i] = rigSerialMenu->AppendRadioItem(rateMenuId, rateString, wxT("Description?"));

        if (rigSerialRate == rate_i) {
            rigSerialMenuItems[rate_i]->Check(true);
        }

        rateMenuId++;
    }

    pMenu->AppendSubMenu(rigSerialMenu, wxT("Serial Rate"));

    rigPortMenuItem = pMenu->Append(wxID_RIG_PORT, wxT("Control Port"));

    return pMenu;
}
#endif


ScopeCanvas *AppFrame::makeScopeCanvas(wxPanel *parent, const wxGLAttributes &attribList) {
    ScopeCanvas *pCanvas = new ScopeCanvas(parent, attribList);
    pCanvas->setHelpTip("Audio Visuals, drag left/right to toggle Scope or Spectrum, 'B' to toggle decibels display.");
    pCanvas->SetMinSize(wxSize(128, -1));
    return pCanvas;
}

wxMenu *AppFrame::makeDisplayMenu() {

    //Add Display menu
    wxMenu *dispMenu = new wxMenu;
    wxMenu *fontMenu = new wxMenu;

    int fontScale = wxGetApp().getConfig()->getFontScale();

    fontMenu->AppendRadioItem(wxID_DISPLAY_BASE, "Default")->Check(GLFont::GLFONT_SCALE_NORMAL == fontScale);
    fontMenu->AppendRadioItem(wxID_DISPLAY_BASE + 1, "1.5x")->Check(GLFont::GLFONT_SCALE_MEDIUM == fontScale);
    fontMenu->AppendRadioItem(wxID_DISPLAY_BASE + 2, "2.0x")->Check(GLFont::GLFONT_SCALE_LARGE == fontScale);

    dispMenu->AppendSubMenu(fontMenu, "&Text Size");

    wxMenu *themeMenu = new wxMenu;

    int themeId = wxGetApp().getConfig()->getTheme();

    themeMenu->AppendRadioItem(wxID_THEME_DEFAULT, "Default")->Check(themeId==COLOR_THEME_DEFAULT);
    themeMenu->AppendRadioItem(wxID_THEME_RADAR, "RADAR")->Check(themeId==COLOR_THEME_RADAR);
    themeMenu->AppendRadioItem(wxID_THEME_BW, "Black & White")->Check(themeId==COLOR_THEME_BW);
    themeMenu->AppendRadioItem(wxID_THEME_SHARP, "Sharp")->Check(themeId==COLOR_THEME_SHARP);
    themeMenu->AppendRadioItem(wxID_THEME_RAD, "Rad")->Check(themeId==COLOR_THEME_RAD);
    themeMenu->AppendRadioItem(wxID_THEME_TOUCH, "Touch")->Check(themeId==COLOR_THEME_TOUCH);
    themeMenu->AppendRadioItem(wxID_THEME_HD, "HD")->Check(themeId==COLOR_THEME_HD);

    dispMenu->AppendSubMenu(themeMenu, wxT("&Color Scheme"));

    hideBookmarksItem = dispMenu->AppendCheckItem(wxID_DISPLAY_BOOKMARKS, wxT("Hide Bookmarks"));
    hideBookmarksItem->Check(!wxGetApp().getConfig()->getBookmarksVisible());

    return dispMenu;
}

wxMenu *AppFrame::makeAudioSampleRateMenu() {
    // Audio Sample Rates
    wxMenu *pMenu = new wxMenu;

    auto outputDevices = wxGetApp().getDemodMgr().getOutputDevices();

#define NUM_RATES_DEFAULT 4
    unsigned int desired_rates[NUM_RATES_DEFAULT] = { 48000, 44100, 96000, 192000 };

    for (auto mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
        unsigned int desired_rate = 0;
        unsigned int desired_rank = NUM_RATES_DEFAULT + 1;

        for (auto srate = mdevices_i->second.sampleRates.begin(); srate != mdevices_i->second.sampleRates.end();
             srate++) {
            for (unsigned int i = 0; i < NUM_RATES_DEFAULT; i++) {
                if (desired_rates[i] == (*srate)) {
                    if (desired_rank > i) {
                        desired_rank = i;
                        desired_rate = (*srate);
                    }
                }
            }
        }

        if (desired_rank > NUM_RATES_DEFAULT) {
            desired_rate = mdevices_i->second.sampleRates.back();
        }
        AudioThread::deviceSampleRate[mdevices_i->first] = desired_rate;
    }

    for (auto mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
        int menu_id = wxID_AUDIO_BANDWIDTH_BASE + wxID_AUDIO_DEVICE_MULTIPLIER * mdevices_i->first;
        wxMenu *subMenu = new wxMenu;
        pMenu->AppendSubMenu(subMenu, mdevices_i->second.name, wxT("Description?"));

        int j = 0;
        for (auto srate = mdevices_i->second.sampleRates.begin(); srate != mdevices_i->second.sampleRates.end();
             srate++) {
            stringstream srateName;
            srateName << ((float) (*srate) / 1000.0f) << "kHz";
            wxMenuItem *itm = subMenu->AppendRadioItem(menu_id + j, srateName.str(), wxT("Description?"));

            if ((int)(*srate) == AudioThread::deviceSampleRate[mdevices_i->first]) {
                itm->Check(true);
            }
            audioSampleRateMenuItems[menu_id + j] = itm;

            j++;
        }
    }
    return pMenu;
}

MeterCanvas *AppFrame::makeWaterfallSpeedMeter(wxWindow *parent, const wxGLAttributes &attribList) {
    MeterCanvas *pCanvas = new MeterCanvas(parent, attribList);
    pCanvas->setHelpTip("Waterfall speed, click or drag to adjust (max 1024 lines per second)");
    pCanvas->setMax(sqrt(1024));
    pCanvas->setLevel(sqrt(DEFAULT_WATERFALL_LPS));
    pCanvas->setShowUserInput(false);
    pCanvas->SetMinSize(wxSize(12, 24));
    return pCanvas;
}

WaterfallCanvas *AppFrame::makeWaterfall(wxWindow *parent, const wxGLAttributes &attribList) {
    WaterfallCanvas *pCanvas = new WaterfallCanvas(parent, attribList);
    pCanvas->setup(DEFAULT_FFT_SIZE, DEFAULT_MAIN_WATERFALL_LINES_NB);
    return pCanvas;
}

MeterCanvas * AppFrame::makeSpectrumAvgMeter(wxWindow *parent, const wxGLAttributes &attribList) {
    MeterCanvas *pCanvas = new MeterCanvas(parent, attribList);
    pCanvas->setHelpTip("Spectrum averaging speed, click or drag to adjust.");
    pCanvas->setMax(1.0);
    pCanvas->setLevel(0.65f);
    pCanvas->setShowUserInput(false);
    pCanvas->SetMinSize(wxSize(12, 24));
    return pCanvas;
}

SpectrumCanvas *AppFrame::makeSpectrumCanvas(wxWindow *parent, const wxGLAttributes &attribList) {
    SpectrumCanvas *pCanvas = new SpectrumCanvas(parent, attribList);
    pCanvas->setShowDb(true);
    pCanvas->setUseDBOfs(true);
    pCanvas->setScaleFactorEnabled(true);
    return pCanvas;
}

ModeSelectorCanvas *AppFrame::makePeakHoldButton(wxWindow *parent, const wxGLAttributes &attribList) {
    ModeSelectorCanvas *pCanvas = new ModeSelectorCanvas(parent, attribList);
    pCanvas->addChoice(1, "P");
    pCanvas->setPadding(-1, -1);
    pCanvas->setHighlightColor(RGBA4f(0.2f, 0.8f, 0.2f));
    pCanvas->setHelpTip("Peak Hold Toggle");
    pCanvas->setToggleMode(true);
    pCanvas->setSelection(-1);
    pCanvas->SetMinSize(wxSize(12, 24));
    return pCanvas;
}

ModeSelectorCanvas *AppFrame::makeModemMuteButton(wxWindow *parent, const wxGLAttributes &attribList) {
    ModeSelectorCanvas *pCanvas = new ModeSelectorCanvas(parent, attribList);
    pCanvas->addChoice(1, "M");
    pCanvas->setPadding(-1, -1);
    pCanvas->setHighlightColor(RGBA4f(0.8f, 0.2f, 0.2f));
    pCanvas->setHelpTip("Demodulator Mute Toggle");
    pCanvas->setToggleMode(true);
    pCanvas->setSelection(-1);
    pCanvas->SetMinSize(wxSize(12, 28));
    return pCanvas;
}

ModeSelectorCanvas *AppFrame::makeSoloModeButton(wxWindow *parent, const wxGLAttributes &attribList) {
    ModeSelectorCanvas *pCanvas = new ModeSelectorCanvas(parent, attribList);
    pCanvas->addChoice(1, "S");
    pCanvas->setPadding(-1, -1);
    pCanvas->setHighlightColor(RGBA4f(0.8f, 0.8f, 0.2f));
    pCanvas->setHelpTip("Solo Mode Toggle");
    pCanvas->setToggleMode(true);
    pCanvas->setSelection(-1);
    pCanvas->SetMinSize(wxSize(12, 28));
    return pCanvas;
}

MeterCanvas *AppFrame::makeModemGainMeter(wxWindow *parent, const wxGLAttributes &attribList) {
    MeterCanvas *pCanvas = new MeterCanvas(parent, attribList);
    pCanvas->setMax(2.0);
    pCanvas->setHelpTip("Current Demodulator Gain Level.  Click / Drag to set Gain level.");
    pCanvas->setShowUserInput(false);
    pCanvas->SetMinSize(wxSize(13, 24));
    return pCanvas;
}

TuningCanvas *AppFrame::makeModemTuner(wxWindow *parent, const wxGLAttributes &attribList) {
    TuningCanvas *pCanvas = new TuningCanvas(parent, attribList);
    pCanvas->SetMinClientSize(wxSize(200, 28));
    return pCanvas;
}

ModeSelectorCanvas * AppFrame::makeDeltaLockButton(wxWindow *parent, const wxGLAttributes &attribList) {
    ModeSelectorCanvas *pCanvas = new ModeSelectorCanvas(parent, attribList);
    pCanvas->addChoice(1, "V");
    pCanvas->setPadding(-1, -1);
    pCanvas->setHighlightColor(RGBA4f(0.8f, 0.8f, 0.2f));
    pCanvas->setHelpTip("Delta Lock Toggle (V) - Enable to lock modem relative to center frequency.");
    pCanvas->setToggleMode(true);
    pCanvas->setSelection(-1);
    pCanvas->SetMinSize(wxSize(20, 28));
    return pCanvas;
}

MeterCanvas *AppFrame::makeSignalMeter(wxWindow *parent, const wxGLAttributes &attribList) {
    MeterCanvas *pCanvas = new MeterCanvas(parent, attribList);
    pCanvas->setMax(DEMOD_SIGNAL_MAX);
    pCanvas->setMin(DEMOD_SIGNAL_MIN);
    pCanvas->setLevel(DEMOD_SIGNAL_MIN);
    pCanvas->setInputValue(DEMOD_SIGNAL_MIN);
    pCanvas->setHelpTip("Current Signal Level.  Click / Drag to set Squelch level.  Right-Click to Auto-Zero Squelch");
    pCanvas->SetMinSize(wxSize(12, 24));
    return pCanvas;
}

SpectrumCanvas *AppFrame::makeDemodSpectrumCanvas(wxWindow *parent, const wxGLAttributes &attribList) {
    SpectrumCanvas *pCanvas = new SpectrumCanvas(parent, attribList);
    pCanvas->setView(wxGetApp().getConfig()->getCenterFreq(), 300000);
    return pCanvas;
}

WaterfallCanvas *AppFrame::makeWaterfallCanvas(wxWindow *parent, const wxGLAttributes &attribList) {
    WaterfallCanvas *pCanvas = new WaterfallCanvas(parent, attribList);
    pCanvas->setup(DEFAULT_DMOD_FFT_SIZE, DEFAULT_DEMOD_WATERFALL_LINES_NB);
    pCanvas->setView(wxGetApp().getConfig()->getCenterFreq(), 300000);
    pCanvas->setMinBandwidth(8000);
    pCanvas->getVisualDataQueue()->set_max_num_items(3);
    pCanvas->setLinesPerSecond((int)(DEFAULT_DEMOD_WATERFALL_LINES_NB / DEMOD_WATERFALL_DURATION_IN_SECONDS));
    return pCanvas;
}

ModeSelectorCanvas *AppFrame::makeModemSelectorPanel(wxWindow *parent, const wxGLAttributes &attribList) {
    vector<string> modemList = {"FM", "FMS", "NBFM", "AM", "LSB", "USB", "DSB", "I/Q" };

#ifdef CUBICSDR_MODEM_EXCLUDE
    std::string excludeListStr = "" CUBICSDR_MODEM_EXCLUDE;
    std::vector<std::string> excludeList = str_explode(",",excludeListStr);
    for (auto ex_i : excludeList) {
        std::vector<std::string>::iterator found_i = std::find(modemList.begin(),modemList.end(),ex_i);
        if (found_i != modemList.end()) {
            modemList.erase(found_i);
        }
    }
#endif

    ModeSelectorCanvas *pCanvas = new ModeSelectorCanvas(parent, attribList);

    for (auto mt_i : modemList) {
        pCanvas->addChoice(mt_i);
    }

#ifdef CUBICSDR_MODEM_EXCLUDE
    demodModeSelector->setHelpTip("Use buttons to choose modulation type.");
#else
    pCanvas->setHelpTip("Choose modulation type: Frequency Modulation (Hotkey F), Amplitude Modulation (A) and Lower (L), Upper (U), Double Side-Band and more.");
#endif

    pCanvas->SetMinSize(wxSize(50, -1));
    pCanvas->SetMaxSize(wxSize(50, -1));

    return pCanvas;
}

AppFrame::~AppFrame() {

    waterfallDataThread->terminate();
    t_FFTData->join();
}

wxMenu *AppFrame::makeFileMenu() {
    
    wxMenu *menu = new wxMenu;
#ifndef __APPLE__ 
#ifdef CUBICSDR_ENABLE_ABOUT_DIALOG
    menu->Append(wxID_ABOUT_CUBICSDR, "About " CUBICSDR_INSTALL_NAME);
#endif
#endif
    menu->Append(wxID_SDR_DEVICES, "SDR Devices");
    menu->AppendSeparator();
    menu->Append(wxID_SDR_START_STOP, "Stop / Start Device");
    menu->AppendSeparator();

    wxMenu *sessionMenu = new wxMenu;
    
    sessionMenu->Append(wxID_OPEN, "&Open Session");
    sessionMenu->Append(wxID_SAVE, "&Save Session");
    sessionMenu->Append(wxID_SAVEAS, "Save Session &As..");
    sessionMenu->AppendSeparator();
    sessionMenu->Append(wxID_RESET, "&Reset Session");

    menu->AppendSubMenu(sessionMenu, "Session");

    menu->AppendSeparator();

    wxMenu *bookmarkMenu = new wxMenu;
    
    bookmarkMenu->Append(wxID_OPEN_BOOKMARKS, "Open Bookmarks");
	bookmarkMenu->Append(wxID_SAVE_BOOKMARKS, "Save Bookmarks");
	bookmarkMenu->Append(wxID_SAVEAS_BOOKMARKS, "Save Bookmarks As..");
	bookmarkMenu->AppendSeparator();
	bookmarkMenu->Append(wxID_RESET_BOOKMARKS, "Reset Bookmarks");

    menu->AppendSubMenu(bookmarkMenu, "Bookmarks");
    
#ifndef __APPLE__
    menu->AppendSeparator();
    menu->Append(wxID_CLOSE);
#else
#ifdef CUBICSDR_ENABLE_ABOUT_DIALOG
    if (wxApp::s_macAboutMenuItemId != wxID_NONE) {
        wxString aboutLabel;
        aboutLabel.Printf(_("About %s"), CUBICSDR_INSTALL_NAME);
        menu->Append(wxApp::s_macAboutMenuItemId, aboutLabel);
    }
#endif
#endif

    return menu;
}

wxMenu *AppFrame::makeRecordingMenu() {
	
	recordingMenuItems.clear();

	wxMenu *menu = new wxMenu;

	recordingMenuItems[wxID_RECORDING_PATH] = menu->Append(wxID_RECORDING_PATH, getSettingsLabel("Set Recording Path", "<Not Set>"));

	menu->AppendSeparator();

	//Squelch options as sub-menu:
	wxMenu *subMenu = new wxMenu;
	recordingMenuItems[wxID_RECORDING_SQUELCH_BASE] = menu->AppendSubMenu(subMenu, "Squelch");

	recordingMenuItems[wxID_RECORDING_SQUELCH_SILENCE] = subMenu->AppendRadioItem(wxID_RECORDING_SQUELCH_SILENCE, "Record Silence", 
		"Record below squelch-break audio as silence, i.e records as the user may hear.");
	recordingMenuItems[wxID_RECORDING_SQUELCH_SKIP] = subMenu->AppendRadioItem(wxID_RECORDING_SQUELCH_SKIP, "Skip Silence", 
		"Do not record below squelch-break audio, i.e squelch-break audio parts are packed together.");
	recordingMenuItems[wxID_RECORDING_SQUELCH_ALWAYS] = subMenu->AppendRadioItem(wxID_RECORDING_SQUELCH_ALWAYS, "Record Always", 
		"Record everything irrespective of the squelch level.");
	
	recordingMenuItems[wxID_RECORDING_FILE_TIME_LIMIT] = menu->Append(wxID_RECORDING_FILE_TIME_LIMIT, getSettingsLabel("File time limit", "<Not Set>"), 
		"Creates a new file automatically, each time the recording lasts longer than the limit, named according to the current time.");

	recordingMenuItems[wxID_RECORDING_SQUELCH_SILENCE]->Check(true);

	return menu;
}

void AppFrame::updateRecordingMenu() {

	// Recording path:
	std::string recPath = wxGetApp().getConfig()->getRecordingPath();
	if (recPath.length() > 32) {
		recPath = "..." + recPath.substr(recPath.length() - 32, 32);
	}

	recordingMenuItems[wxID_RECORDING_PATH]->SetItemLabel(getSettingsLabel("Set Recording Path", recPath.empty() ? "<Not Set>" : recPath));

	//Squelch options:
	int squelchEnumValue = wxGetApp().getConfig()->getRecordingSquelchOption();

	if (squelchEnumValue == AudioSinkFileThread::SQUELCH_RECORD_SILENCE) {

		recordingMenuItems[wxID_RECORDING_SQUELCH_SILENCE]->Check(true);
		recordingMenuItems[wxID_RECORDING_SQUELCH_BASE]->SetItemLabel(getSettingsLabel("Squelch", "Record Silence"));

	} else if (squelchEnumValue == AudioSinkFileThread::SQUELCH_SKIP_SILENCE) {

		recordingMenuItems[wxID_RECORDING_SQUELCH_SKIP]->Check(true);
		recordingMenuItems[wxID_RECORDING_SQUELCH_BASE]->SetItemLabel(getSettingsLabel("Squelch", "Skip Silence"));

	} else if (squelchEnumValue == AudioSinkFileThread::SQUELCH_RECORD_ALWAYS) {

		recordingMenuItems[wxID_RECORDING_SQUELCH_ALWAYS]->Check(true);
		recordingMenuItems[wxID_RECORDING_SQUELCH_BASE]->SetItemLabel(getSettingsLabel("Squelch", "Record Always"));
	}
	else {
		recordingMenuItems[wxID_RECORDING_SQUELCH_SILENCE]->Check(true);
		recordingMenuItems[wxID_RECORDING_SQUELCH_BASE]->SetItemLabel(getSettingsLabel("Squelch", "Record Silence"));

	}

	//File time limit:
	int fileTimeLimitSeconds = wxGetApp().getConfig()->getRecordingFileTimeLimit();

	if (fileTimeLimitSeconds <= 0) {
		
		recordingMenuItems[wxID_RECORDING_FILE_TIME_LIMIT]->SetItemLabel(getSettingsLabel("File time limit","<Not Set>"));
	}
	else {
		recordingMenuItems[wxID_RECORDING_FILE_TIME_LIMIT]->SetItemLabel(getSettingsLabel("File time limit",
			std::to_string(fileTimeLimitSeconds), "s"));
	}
}

void AppFrame::initDeviceParams(SDRDeviceInfo *devInfo) {
    this->devInfo = devInfo;
    deviceChanged.store(true);
}

void AppFrame::notifyDeviceChanged() {
    deviceChanged.store(true);
}

void AppFrame::handleUpdateDeviceParams() {
    
    if (!deviceChanged.load() || devInfo == nullptr) {
        return;
    }
    
    int i = 0;
    SoapySDR::Device *soapyDev = devInfo->getSoapyDevice();
    
    // Build settings menu
    wxMenu *newSettingsMenu = new wxMenu;
    showTipMenuItem = newSettingsMenu->AppendCheckItem(wxID_SET_TIPS, "Show Hover Tips");
    showTipMenuItem->Check(wxGetApp().getConfig()->getShowTips());

    // CPU usage menu:
    performanceMenuItems.clear();

    wxMenu *subMenu = new wxMenu;

    performanceMenuItems[wxID_PERF_BASE + (int)AppConfig::PERF_HIGH] = subMenu->AppendRadioItem(wxID_PERF_BASE + (int)AppConfig::PERF_HIGH, "High (+enhanced DSP)");
    performanceMenuItems[wxID_PERF_BASE + (int)AppConfig::PERF_NORMAL] = subMenu->AppendRadioItem(wxID_PERF_BASE + (int)AppConfig::PERF_NORMAL, "Normal");
    performanceMenuItems[wxID_PERF_BASE + (int)AppConfig::PERF_LOW] = subMenu->AppendRadioItem(wxID_PERF_BASE + (int)AppConfig::PERF_LOW, "Low (-slow UI)");

    AppConfig::PerfModeEnum perfMode = wxGetApp().getConfig()->getPerfMode();

    if (perfMode == AppConfig::PERF_HIGH) {
        wxGetApp().setChannelizerType(SDRPostThreadChannelizerType::SDRPostPFBCH2);
    } else {
        wxGetApp().setChannelizerType(SDRPostThreadChannelizerType::SDRPostPFBCH);
    }

    performanceMenuItems[wxID_PERF_BASE + (int)perfMode]->Check(true);

    wxMenuItem* selectedPerfModeItem = performanceMenuItems[wxID_PERF_BASE + (int)perfMode];

    performanceMenuItems[wxID_PERF_CURRENT] = newSettingsMenu->AppendSubMenu(subMenu, "CPU usage");
    performanceMenuItems[wxID_PERF_CURRENT]->SetItemLabel(getSettingsLabel("CPU usage", selectedPerfModeItem->GetItemLabel().ToStdString()));
   
    newSettingsMenu->AppendSeparator();

    settingsMenuItems.clear();

    settingsMenuItems[wxID_SET_DB_OFFSET] = newSettingsMenu->Append(wxID_SET_DB_OFFSET, getSettingsLabel("Power Level Offset",  std::to_string(wxGetApp().getConfig()->getDBOffset()), "dB"));
    settingsMenuItems[wxID_SET_FREQ_OFFSET] =  newSettingsMenu->Append(wxID_SET_FREQ_OFFSET, getSettingsLabel("Frequency Offset", std::to_string(wxGetApp().getOffset() / 1000 ) , "KHz"));

    if (devInfo->hasCORR(SOAPY_SDR_RX, 0)) {
        settingsMenuItems[wxID_SET_PPM] = newSettingsMenu->Append(wxID_SET_PPM, getSettingsLabel("Device PPM", std::to_string(wxGetApp().getPPM()) , "ppm"));
    }

    if (devInfo->getDriver() != "rtlsdr") {
        iqSwapMenuItem = newSettingsMenu->AppendCheckItem(wxID_SET_IQSWAP, "I/Q Swap");
        iqSwapMenuItem->Check(wxGetApp().getSDRThread()->getIQSwap());
    }

    agcMenuItem = nullptr;
    if (soapyDev->listGains(SOAPY_SDR_RX, 0).size()) {
        agcMenuItem = newSettingsMenu->AppendCheckItem(wxID_AGC_CONTROL, "Automatic Gain");
        agcMenuItem->Check(wxGetApp().getAGCMode());
    } else if (!wxGetApp().getAGCMode()) {
        wxGetApp().setAGCMode(true);
    }

    //Add an Antenna menu if more than one (RX) antenna, to keep the UI free of useless entries
    antennaNames.clear();
    antennaMenuItems.clear();
    std::vector<std::string> availableAntennas = devInfo->getAntennaNames(SOAPY_SDR_RX, 0);
 
    if (availableAntennas.size() > 1) {
              
        newSettingsMenu->AppendSeparator();

        antennaNames = availableAntennas;

        wxMenu *subMenu = new wxMenu;
        
        int i = 0;
        std::string antennaChecked;
        for (std::string currentAntenna : availableAntennas) {
           
            antennaMenuItems[wxID_ANTENNAS_BASE + i] = subMenu->AppendRadioItem(wxID_ANTENNAS_BASE + i, currentAntenna);

            if (wxGetApp().getAntennaName() == currentAntenna) {
                antennaMenuItems[wxID_ANTENNAS_BASE + i]->Check(true);
                antennaChecked = currentAntenna;
            }

            i++;
        }
        antennaMenuItems[wxID_ANTENNA_CURRENT] = newSettingsMenu->AppendSubMenu(subMenu, "Antenna");
        
        //Change the Antenna label to indicate the current antenna.
        if (!antennaChecked.empty()) {
        
            antennaMenuItems[wxID_ANTENNA_CURRENT]->SetItemLabel(getSettingsLabel("Antenna", antennaChecked));
        }
    }

    //Add an informative, read-only menu entry to display the current TX selected antenna, if any.
    if (devInfo->getAntennaNames(SOAPY_SDR_TX, 0).size() > 1) {

        currentTXantennaName = devInfo->getAntennaName(SOAPY_SDR_TX, 0);
        
        newSettingsMenu->AppendSeparator();
        
        antennaMenuItems[wxID_ANTENNA_CURRENT_TX] = newSettingsMenu->Append(wxID_ANTENNA_CURRENT_TX, getSettingsLabel("TX Antenna", currentTXantennaName));
        antennaMenuItems[wxID_ANTENNA_CURRENT_TX]->Enable(false);
    }

    //Runtime settings part
    SoapySDR::ArgInfoList::const_iterator args_i;
    settingArgs = soapyDev->getSettingInfo();

    if (settingArgs.size()) {
        newSettingsMenu->AppendSeparator();
    }
    //for each Runtime option of index i:
    for (args_i = settingArgs.begin(); args_i != settingArgs.end(); args_i++) {

        SoapySDR::ArgInfo arg = (*args_i);

        std::string currentVal = soapyDev->readSetting(arg.key);
        
		if (arg.type == SoapySDR::ArgInfo::BOOL) {
            wxMenuItem *item = newSettingsMenu->AppendCheckItem(wxID_SETTINGS_BASE+i, arg.name, arg.description);
            item->Check(currentVal=="true");
            i++;
        } else if (arg.type == SoapySDR::ArgInfo::INT) {
            
            settingsMenuItems[wxID_SETTINGS_BASE + i] = newSettingsMenu->Append(wxID_SETTINGS_BASE + i, getSettingsLabel(arg.name, currentVal, arg.units), arg.description);
            i++;
        } else if (arg.type == SoapySDR::ArgInfo::FLOAT) {
            settingsMenuItems[wxID_SETTINGS_BASE + i] = newSettingsMenu->Append(wxID_SETTINGS_BASE + i, getSettingsLabel(arg.name, currentVal, arg.units), arg.description);
            i++;
        } else if (arg.type == SoapySDR::ArgInfo::STRING) {
            if (arg.options.size()) {
                wxMenu *subMenu = new wxMenu;
                int j = 0;
                std::vector<int> subItemsIds;
				//for each of this options
                for (std::string optName : arg.options) {
					//by default the option name is the same as the displayed name.
                    std::string displayName = optName;
                    
					if (arg.optionNames.size()) {
                        displayName = arg.optionNames[j];
                    }
                    wxMenuItem *item = subMenu->AppendRadioItem(wxID_SETTINGS_BASE+i, displayName);
                    subItemsIds.push_back(wxID_SETTINGS_BASE + i);
                    
                    if (currentVal == optName) {
                        item->Check(true);
                    }
                    j++;
                    i++;
                }
                settingsMenuItems[wxID_SETTINGS_BASE + i] = newSettingsMenu->AppendSubMenu(subMenu, getSettingsLabel(arg.name, currentVal, arg.units), arg.description);
                //map subitems ids to their parent item !
                for (int currentSubId : subItemsIds) {
                    settingsMenuItems[currentSubId] = settingsMenuItems[wxID_SETTINGS_BASE + i];
                }
            } else {
                settingsMenuItems[wxID_SETTINGS_BASE + i] = newSettingsMenu->Append(wxID_SETTINGS_BASE + i, getSettingsLabel(arg.name, currentVal, arg.units), arg.description);
                i++;
            }
        }
    }
    settingsIdMax = wxID_SETTINGS_BASE+i;
    
    menuBar->Replace(1, newSettingsMenu, wxT("&Settings"));
    settingsMenu = newSettingsMenu;
    
    // Build/Rebuild the sample rate menu :
    sampleRates = devInfo->getSampleRates(SOAPY_SDR_RX, 0);
    sampleRateMenuItems.clear();
    
    wxMenu *newSampleRateMenu = new wxMenu;
    int ofs = 0;
    
    //Current sample rate, try to keep it as is.
    long sampleRate = wxGetApp().getSampleRate();
   
    long minRate = sampleRates.front();
    long maxRate = sampleRates.back();

    //If it is beyond limits, make device choose a reasonable value
    if (sampleRate < minRate || sampleRate > maxRate) {
        sampleRate = devInfo->getSampleRateNear(SOAPY_SDR_RX, 0, sampleRate);
    }

    //Check if a manual entry was previously set: if so, check its value is still within the limits of the device. If not so, reset it.
    if (manualSampleRate > 0 && 
        (manualSampleRate < minRate || manualSampleRate > maxRate)) {
        manualSampleRate = -1;
    }

    bool checked = false;
    for (vector<long>::iterator i = sampleRates.begin(); i != sampleRates.end(); i++) {

        sampleRateMenuItems[wxID_BANDWIDTH_BASE+ofs] = newSampleRateMenu->AppendRadioItem(wxID_BANDWIDTH_BASE+ofs, frequencyToStr(*i));
        
        if (sampleRate == (*i)) {
            sampleRateMenuItems[wxID_BANDWIDTH_BASE+ofs]->Check(true);
            checked = true;
        }
        ofs++;
    }
    
    //Add a manual sample value radio button, but disabled by default in case the user 
    //never ever uses manual entry.
    if (manualSampleRate <= 0) {
        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL] = newSampleRateMenu->AppendRadioItem(wxID_BANDWIDTH_MANUAL, wxT("Manual :  N/A"));
        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->Enable(false);
    }
    else {
        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL] = newSampleRateMenu->AppendRadioItem(wxID_BANDWIDTH_MANUAL, wxT("Manual :  ") + frequencyToStr(manualSampleRate));
        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->Enable(true);
    }

    //We apply the current sample rate after all 
    if (!checked) {
        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->Check(true);
    }

    //Append a normal button (NOT a radio-button) for manual entry dialog at the end
    newSampleRateMenu->AppendSeparator();
    sampleRateMenuItems[wxID_BANDWIDTH_MANUAL_DIALOG] = newSampleRateMenu->Append(wxID_BANDWIDTH_MANUAL_DIALOG, wxT("Manual Entry..."));

    menuBar->Replace(2, newSampleRateMenu, wxT("Sample &Rate"));
    sampleRateMenu = newSampleRateMenu;

    if (!wxGetApp().getAGCMode()) {
        gainSpacerItem->Show(true);
        gainSizerItem->Show(true);
        gainSizerItem->SetMinSize(devInfo->getSoapyDevice()->listGains(SOAPY_SDR_RX,0).size()*50,0);
        demodTray->Layout();
    } else {
        gainSpacerItem->Show(false);
        gainSizerItem->Show(false);
        demodTray->Layout();
    }


#if USE_HAMLIB
    if (wxGetApp().getConfig()->getRigEnabled() && !wxGetApp().rigIsActive()) {
        enableRig();
        rigEnableMenuItem->Check(true);
    }
    
    std::string deviceId = devInfo->getDeviceId();
    DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(deviceId);

    if (wxGetApp().rigIsActive()) {
        rigSDRIF = devConfig->getRigIF(rigModel);
        if (rigSDRIF) {
            wxGetApp().lockFrequency(rigSDRIF);
        } else {
            wxGetApp().unlockFrequency();
        }
    }
#endif
    
    deviceChanged.store(false);
}

#ifdef USE_HAMLIB
void AppFrame::enableRig() {
    wxGetApp().stopRig();
    wxGetApp().initRig(rigModel, rigPort, rigSerialRate);

    if (devInfo != nullptr) {
        std::string deviceId = devInfo->getDeviceId();
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(deviceId);
        rigSDRIF = devConfig->getRigIF(rigModel);
        if (rigSDRIF) {
            wxGetApp().lockFrequency(rigSDRIF);
        } else {
            wxGetApp().unlockFrequency();
        }
    } else {
        wxGetApp().unlockFrequency();
    }
    
    wxGetApp().getConfig()->setRigEnabled(true);
}

void AppFrame::disableRig() {
    wxGetApp().stopRig();
    wxGetApp().unlockFrequency();
    wxGetApp().getConfig()->setRigEnabled(false);
}

void AppFrame::setRigControlPort(std::string portName) {
    if (rigPortDialog == nullptr) {
        return;
    }
    if (portName != "") {
        rigPort = portName;

        wxGetApp().stopRig();
        wxGetApp().initRig(rigModel, rigPort, rigSerialRate);
        
        Refresh();
    }
    rigPortDialog->EndModal(0);
    delete rigPortDialog;
    rigPortDialog = nullptr;
}


void AppFrame::dismissRigControlPortDialog() {
    rigPortDialog->EndModal(0);
    delete rigPortDialog;
    rigPortDialog = nullptr;
}

#endif


void AppFrame::OnMenu(wxCommandEvent &event) {
    actionOnMenuAbout(event)
    || actionOnMenuSDRStartStop(event)
    || actionOnMenuPerformance(event)
    || actionOnMenuTips(event)
    || actionOnMenuIQSwap(event)
    || actionOnMenuFreqOffset(event)
    || actionOnMenuDBOffset(event)
    || actionOnMenuAGC(event)
    || actionOnMenuSDRDevices(event)
    || actionOnMenuSetPPM(event)
    || actionOnMenuLoadSave(event)
    || actionOnMenuReset(event)
    || actionOnMenuClose(event)
    || actionOnMenuSettings(event)
    || actionOnMenuSampleRate(event)
    || actionOnMenuAudioSampleRate(event)
    || actionOnMenuRecording(event)
    || actionOnMenuDisplay(event)
    //Optional : Rig
    || actionOnMenuRig(event);
}


bool AppFrame::actionOnMenuDisplay(wxCommandEvent& event) {

    //by default, is managed.
    bool bManaged = true;

    if (event.GetId() == wxID_THEME_DEFAULT) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_DEFAULT);
    }
    else if (event.GetId() == wxID_THEME_SHARP) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_SHARP);
    }
    else if (event.GetId() == wxID_THEME_BW) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_BW);
    }
    else if (event.GetId() == wxID_THEME_RAD) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_RAD);
    }
    else if (event.GetId() == wxID_THEME_TOUCH) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_TOUCH);
    }
    else if (event.GetId() == wxID_THEME_HD) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_HD);
    }
    else if (event.GetId() == wxID_THEME_RADAR) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_RADAR);
    }
    //Display : font sizes
    else if (event.GetId() == wxID_DISPLAY_BASE) {
        GLFont::setScale(GLFont::GLFONT_SCALE_NORMAL);
    }
    else if (event.GetId() == wxID_DISPLAY_BASE + 1) {
        GLFont::setScale(GLFont::GLFONT_SCALE_MEDIUM);
    }
    else if (event.GetId() == wxID_DISPLAY_BASE + 2) {
        GLFont::setScale(GLFont::GLFONT_SCALE_LARGE);
    }
    else if (event.GetId() == wxID_DISPLAY_BOOKMARKS) {
        if (hideBookmarksItem->IsChecked()) {
            bookmarkSplitter->Unsplit(bookmarkView);
            bookmarkSplitter->Layout();
        }
        else {
            bookmarkSplitter->SplitVertically(bookmarkView, mainVisSplitter, wxGetApp().getConfig()->getBookmarkSplit());
            bookmarkSplitter->Layout();
        }
    }
    else {
        bManaged = false;
    }

    //update theme choice in children elements:  
    if (event.GetId() >= wxID_THEME_DEFAULT && event.GetId() <= wxID_THEME_RADAR) {
       
        gainCanvas->setThemeColors();
        modemProps->updateTheme();
        bookmarkView->updateTheme();
    }

    //force all windows refresh
    if (bManaged) {
        Refresh();
    }

    return bManaged;
}

bool AppFrame::actionOnMenuReset(wxCommandEvent& event) {

    if (event.GetId() == wxID_RESET) {

        wxGetApp().getDemodMgr().terminateAll();
        wxGetApp().setFrequency(100000000);
        wxGetApp().getDemodMgr().setLastDemodulatorType("FM");
        demodModeSelector->setSelection(1);
        wxGetApp().getDemodMgr().setLastMuted(false);
        wxGetApp().getDemodMgr().setLastBandwidth(DEFAULT_DEMOD_BW);
        wxGetApp().getDemodMgr().setLastGain(1.0);
        wxGetApp().getDemodMgr().setLastSquelchLevel(-100);
        waterfallCanvas->setBandwidth(wxGetApp().getSampleRate());
        waterfallCanvas->setCenterFrequency(wxGetApp().getFrequency());
        spectrumCanvas->setBandwidth(wxGetApp().getSampleRate());
        spectrumCanvas->setCenterFrequency(wxGetApp().getFrequency());
        waterfallDataThread->setLinesPerSecond(DEFAULT_WATERFALL_LPS);
        waterfallCanvas->setLinesPerSecond(DEFAULT_WATERFALL_LPS);
        waterfallSpeedMeter->setLevel(sqrt(DEFAULT_WATERFALL_LPS));
        wxGetApp().getSpectrumProcessor()->setFFTAverageRate(0.65f);
        spectrumAvgMeter->setLevel(0.65f);

        SetTitle(CUBICSDR_TITLE);
        currentSessionFile = "";
		currentBookmarkFile = "";
        bookmarkSplitter->Unsplit(bookmarkView);
        bookmarkSplitter->SplitVertically(bookmarkView, mainVisSplitter, wxGetApp().getConfig()->getBookmarkSplit());
        hideBookmarksItem->Check(false);
        //force all windows refresh
        Refresh();

        return true;
    }

    return false;
}

bool AppFrame::actionOnMenuAbout(wxCommandEvent& event) {

#ifdef __APPLE__
    if (event.GetId() == wxApp::s_macAboutMenuItemId) {
#else 
    if (event.GetId() == wxID_ABOUT_CUBICSDR) {
#endif
        if (aboutDlg != nullptr) {
            aboutDlg->Raise();
            aboutDlg->SetFocus();
        }
        else {
            aboutDlg = new AboutDialog(NULL);
            aboutDlg->Connect(wxEVT_CLOSE_WINDOW, wxCommandEventHandler(AppFrame::OnAboutDialogClose), NULL, this);

            aboutDlg->Show();
        }

        return true;
    }

    return false;
}

bool AppFrame::actionOnMenuSettings(wxCommandEvent& event) {

    int antennaIdMax = wxID_ANTENNAS_BASE + antennaNames.size();
    
    if (event.GetId() >= wxID_ANTENNAS_BASE && event.GetId() < antennaIdMax) {

        wxGetApp().setAntennaName(antennaNames[event.GetId() - wxID_ANTENNAS_BASE]);
      
        antennaMenuItems[wxID_ANTENNA_CURRENT]->SetItemLabel(getSettingsLabel("Antenna", wxGetApp().getAntennaName()));
        return true;
    } 
    else if (event.GetId() >= wxID_SETTINGS_BASE && event.GetId() < settingsIdMax) {

        int setIdx = event.GetId() - wxID_SETTINGS_BASE;
        int menuIdx = 0;

        for (std::vector<SoapySDR::ArgInfo>::iterator arg_i = settingArgs.begin(); arg_i != settingArgs.end(); arg_i++) {
            SoapySDR::ArgInfo &arg = (*arg_i);

            if (arg.type == SoapySDR::ArgInfo::STRING && arg.options.size() && setIdx >= menuIdx && setIdx < menuIdx + (int)arg.options.size()) {
                int optIdx = setIdx - menuIdx;
                wxGetApp().getSDRThread()->writeSetting(arg.key, arg.options[optIdx]);
                
                //update parent menu item label to display the current value
                settingsMenuItems[menuIdx + wxID_SETTINGS_BASE]->SetItemLabel(getSettingsLabel(arg.name, arg.options[optIdx], arg.units));             
                break;
            }
            else if (arg.type == SoapySDR::ArgInfo::STRING && arg.options.size()) {
                menuIdx += arg.options.size();
            }
            else if (menuIdx == setIdx) {
                if (arg.type == SoapySDR::ArgInfo::BOOL) {
                    wxGetApp().getSDRThread()->writeSetting(arg.key, (wxGetApp().getSDRThread()->readSetting(arg.key) == "true") ? "false" : "true");
                    break;
                }
                else if (arg.type == SoapySDR::ArgInfo::STRING) {
                    wxString stringVal = wxGetTextFromUser(arg.description, arg.name, wxGetApp().getSDRThread()->readSetting(arg.key));

                    settingsMenuItems[menuIdx + wxID_SETTINGS_BASE]->SetItemLabel(getSettingsLabel(arg.name, stringVal.ToStdString(), arg.units));

                    if (stringVal.ToStdString() != "") {
                        wxGetApp().getSDRThread()->writeSetting(arg.key, stringVal.ToStdString());
                    }
                    break;
                }
                else if (arg.type == SoapySDR::ArgInfo::INT) {
                    int currentVal;
                    try {
                        currentVal = std::stoi(wxGetApp().getSDRThread()->readSetting(arg.key));
                    }
                    catch (std::invalid_argument e) {
                        currentVal = 0;
                    }
                    int intVal = wxGetNumberFromUser(arg.description, arg.units, arg.name, currentVal, arg.range.minimum(), arg.range.maximum(), this);

                    settingsMenuItems[menuIdx + wxID_SETTINGS_BASE]->SetItemLabel(getSettingsLabel(arg.name, std::to_string(intVal), arg.units));

                    if (intVal != -1) {
                        wxGetApp().getSDRThread()->writeSetting(arg.key, std::to_string(intVal));
                    }
                    break;
                }
                else if (arg.type == SoapySDR::ArgInfo::FLOAT) {
                    wxString floatVal = wxGetTextFromUser(arg.description, arg.name, wxGetApp().getSDRThread()->readSetting(arg.key));
                    try {
                        wxGetApp().getSDRThread()->writeSetting(arg.key, floatVal.ToStdString());
                    }
                    catch (std::invalid_argument e) {
                        // ...
                    }
                    settingsMenuItems[menuIdx + wxID_SETTINGS_BASE]->SetItemLabel(getSettingsLabel(arg.name, floatVal.ToStdString(), arg.units));
                    break;
                }
                else {
                    menuIdx++;
                }
            }
            else {
                menuIdx++;
            }
        } //end for
		
        return true;
    }

    return false;
}

bool AppFrame::actionOnMenuAGC(wxCommandEvent& event) {

    if (event.GetId() == wxID_AGC_CONTROL) {

        if (wxGetApp().getDevice() == NULL) {
            agcMenuItem->Check(true);
            return true;
        }
        if (!wxGetApp().getAGCMode()) {
            wxGetApp().setAGCMode(true);
            gainSpacerItem->Show(false);
            gainSizerItem->Show(false);
            demodTray->Layout();
        }
        else {
            wxGetApp().setAGCMode(false);
            gainSpacerItem->Show(true);
            gainSizerItem->Show(true);
            gainSizerItem->SetMinSize(wxGetApp().getDevice()->getSoapyDevice()->listGains(SOAPY_SDR_RX, 0).size() * 40, 0);
            demodTray->Layout();
            gainCanvas->updateGainUI();
        }

        //full Refresh, some graphical elements has changed
        Refresh();

        return true;
    }

    return false;
}

bool AppFrame::actionOnMenuSampleRate(wxCommandEvent& event) {

    if (event.GetId() == wxID_BANDWIDTH_MANUAL) {
        wxGetApp().setSampleRate(manualSampleRate);
        return true;
    }
    else if (event.GetId() == wxID_BANDWIDTH_MANUAL_DIALOG) {

        int rateHigh = 0, rateLow = 0;

        SDRDeviceInfo *dev = wxGetApp().getDevice();
        if (dev != nullptr) {

            std::vector<long> sampleRates = dev->getSampleRates(SOAPY_SDR_RX, 0);

            //default
            rateLow = MANUAL_SAMPLE_RATE_MIN;
            rateHigh = MANUAL_SAMPLE_RATE_MAX;

            if (sampleRates.size()) {
                rateLow = sampleRates.front();
                rateHigh = sampleRates.back();
            }

            long bw = wxGetNumberFromUser("\n" + dev->getName() + "\n\n  "
                + "min: " + std::to_string(rateLow) + " Hz"
                + ", max: " + std::to_string(rateHigh) + " Hz\n",
                "Sample Rate in Hz",
                "Manual Sample Rate Entry",
                //If a manual sample rate has already been input, recall this one.
                manualSampleRate > 0 ? manualSampleRate : wxGetApp().getSampleRate(),
                rateLow,
                rateHigh,
                this);

            if (bw != -1) {

                manualSampleRate = bw;
                sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->Enable(true);

                sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->SetItemLabel(wxT("Manual :  ") + frequencyToStr(manualSampleRate));
                sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->Check(true);
                wxGetApp().setSampleRate(manualSampleRate);
            }
        }

        return true;
    }
    else if (event.GetId() >= wxID_BANDWIDTH_BASE && event.GetId() < wxID_BANDWIDTH_BASE + (int)sampleRates.size()) {

        wxGetApp().setSampleRate(sampleRates[event.GetId() - wxID_BANDWIDTH_BASE]);
        return true;
    }

    return false;
}

bool AppFrame::actionOnMenuAudioSampleRate(wxCommandEvent& event) {

    if (event.GetId() >= wxID_AUDIO_BANDWIDTH_BASE) {

        int evId = event.GetId();

        auto outputDevices = wxGetApp().getDemodMgr().getOutputDevices();

        int i = 0;
        for (auto mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
            int menu_id = wxID_AUDIO_BANDWIDTH_BASE + wxID_AUDIO_DEVICE_MULTIPLIER * mdevices_i->first;

            int j = 0;
            for (std::vector<unsigned int>::iterator srate = mdevices_i->second.sampleRates.begin(); srate != mdevices_i->second.sampleRates.end();
                srate++) {

                if (evId == menu_id + j) {
                    //audioSampleRateMenuItems[menu_id+j];
                    //std::cout << "Would set audio sample rate on device " << mdevices_i->second.name << " (" << mdevices_i->first << ") to " << (*srate) << "Hz" << std::endl;
                    AudioThread::setDeviceSampleRate(mdevices_i->first, *srate);

                    return true;
                }

                j++;
            }
            i++;
        }
    }

    return false;
}

bool AppFrame::actionOnMenuLoadSave(wxCommandEvent& event) {

    if (event.GetId() == wxID_SAVE) {

        if (!currentSessionFile.empty()) {
            saveSession(currentSessionFile);
        }
        else {
            wxFileDialog saveFileDialog(this, _("Save XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (saveFileDialog.ShowModal() == wxID_CANCEL) {
                return true;
            }
            saveSession(saveFileDialog.GetPath().ToStdString());
        }

        return true;
    }
    else if (event.GetId() == wxID_OPEN) {
        wxFileDialog openFileDialog(this, _("Open XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() == wxID_CANCEL) {
            return true;
        }
        loadSession(openFileDialog.GetPath().ToStdString());

        return true;
    }
    else if (event.GetId() == wxID_SAVEAS) {
        wxFileDialog saveFileDialog(this, _("Save XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            return true;
        }
        saveSession(saveFileDialog.GetPath().ToStdString());

        return true;
    }

	//save mecanic for bookmark files
	else if (event.GetId() == wxID_SAVE_BOOKMARKS) {

		if (!currentBookmarkFile.empty()) {
			wxGetApp().getBookmarkMgr().saveToFile(currentBookmarkFile, false, true);
		}
		else {
			wxFileDialog saveFileDialog(this, _("Save XML Bookmark file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
			if (saveFileDialog.ShowModal() == wxID_CANCEL) {
				return true;
			}

			// Make sure the file name actually ends in .xml
			std::string fileName = saveFileDialog.GetPath().ToStdString();
			std::string lcFileName = fileName;

			std::transform(lcFileName.begin(), lcFileName.end(), lcFileName.begin(), ::tolower);

			if (lcFileName.find_last_of(".xml") != lcFileName.length() - 1) {
				fileName.append(".xml");
			}

			wxGetApp().getBookmarkMgr().saveToFile(fileName, false, true);
			currentBookmarkFile = fileName;
		}

		return true;
	}
	else if (event.GetId() == wxID_OPEN_BOOKMARKS) {

		wxFileDialog openFileDialog(this, _("Open XML Bookmark file"), "", "", "XML files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (openFileDialog.ShowModal() == wxID_CANCEL) {
			return true;
		}
		if (wxGetApp().getBookmarkMgr().loadFromFile(openFileDialog.GetPath().ToStdString(), false, true)) {
			
			wxGetApp().getBookmarkMgr().updateBookmarks();
			wxGetApp().getBookmarkMgr().updateActiveList();

			currentBookmarkFile = openFileDialog.GetPath().ToStdString();
		}
		else {
			//failure at loading.
			currentBookmarkFile = "";
		}

		return true;
	}
	else if (event.GetId() == wxID_SAVEAS_BOOKMARKS) {

		wxFileDialog saveFileDialog(this, _("Save XML Bookmark file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (saveFileDialog.ShowModal() == wxID_CANCEL) {
			return true;
		}

		// Make sure the file name actually ends in .xml
		std::string fileName = saveFileDialog.GetPath().ToStdString();
		std::string lcFileName = fileName;

		std::transform(lcFileName.begin(), lcFileName.end(), lcFileName.begin(), ::tolower);

		if (lcFileName.find_last_of(".xml") != lcFileName.length() - 1) {
			fileName.append(".xml");
		}

		wxGetApp().getBookmarkMgr().saveToFile(fileName, false, true);
		currentBookmarkFile = fileName;

		return true;
	}
	else if (event.GetId() == wxID_RESET_BOOKMARKS) {

        ActionDialog::showDialog(new ActionDialogBookmarkReset());

		return true;
	}

    return false;
}

bool AppFrame::actionOnMenuRecording(wxCommandEvent& event) {

	if (event.GetId() == wxID_RECORDING_PATH) {

		std::string recPath = wxGetApp().getConfig()->getRecordingPath();

		wxDirDialog recPathDialog(this, _("File Path for Recordings"), recPath, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
		if (recPathDialog.ShowModal() == wxID_CANCEL) {
			return true;
		}

		wxGetApp().getConfig()->setRecordingPath(recPathDialog.GetPath().ToStdString());

		updateRecordingMenu();
		return true;

	}
	else if (event.GetId() == wxID_RECORDING_SQUELCH_SILENCE) {

		wxGetApp().getConfig()->setRecordingSquelchOption(AudioSinkFileThread::SQUELCH_RECORD_SILENCE);

		updateRecordingMenu();
		return true;
	}
	else if (event.GetId() == wxID_RECORDING_SQUELCH_SKIP) {

		wxGetApp().getConfig()->setRecordingSquelchOption(AudioSinkFileThread::SQUELCH_SKIP_SILENCE);

		updateRecordingMenu();
		return true;
	}
	else if (event.GetId() == wxID_RECORDING_SQUELCH_ALWAYS) {
		
		wxGetApp().getConfig()->setRecordingSquelchOption(AudioSinkFileThread::SQUELCH_RECORD_ALWAYS);

		updateRecordingMenu();
		return true;
	}
	else if (event.GetId() == wxID_RECORDING_FILE_TIME_LIMIT) {

		int currentFileLimitSeconds = wxGetApp().getConfig()->getRecordingFileTimeLimit();

		long newFileLimit = wxGetNumberFromUser(wxString("\nFile time limit:\n") + 
			"\nCreates a new file automatically, each time the recording lasts longer than the limit, named according to the current time.\n\n  " + 
			+ "min: 0 s (no limit)"
			+ ", max: 36000 s (10 hours)\n",
			"Time in seconds",
			"File Time Limit",
			//If a manual sample rate has already been input, recall this one.
			currentFileLimitSeconds > 0 ? currentFileLimitSeconds : 0,
			0,
			36000,
			this);

		if (newFileLimit != -1) {

			wxGetApp().getConfig()->setRecordingFileTimeLimit((int)newFileLimit);

			updateRecordingMenu();
		}

		return true;
	}

	return false;
}

bool AppFrame::actionOnMenuRig(wxCommandEvent &event) {

    bool bManaged = false;

#ifdef USE_HAMLIB

    bool resetRig = false;
    if (event.GetId() >= wxID_RIG_MODEL_BASE && event.GetId() < wxID_RIG_MODEL_BASE + numRigs) {

        int rigIdx = event.GetId() - wxID_RIG_MODEL_BASE;

        RigList &rl = RigThread::enumerate();
        rigModel = rl[rigIdx]->rig_model;

        if (devInfo != nullptr) {
            std::string deviceId = devInfo->getDeviceId();
            DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(deviceId);

            rigSDRIF = devConfig->getRigIF(rigModel);
            if (rigSDRIF) {
                wxGetApp().lockFrequency(rigSDRIF);
            } else {
                wxGetApp().unlockFrequency();
            }
        } else {
            wxGetApp().unlockFrequency();
        }

        resetRig = true;
        bManaged = true;

        for (auto ri : rigModelMenuItems) {
            ri.second->Check(false);
        }

        rigModelMenuItems[rigModel]->Check(true);
    }

    int rigSerialIdMax = wxID_RIG_SERIAL_BASE + rigSerialRates.size();

    if (event.GetId() >= wxID_RIG_SERIAL_BASE && event.GetId() < rigSerialIdMax) {
        int serialIdx = event.GetId() - wxID_RIG_SERIAL_BASE;
        rigSerialRate = rigSerialRates[serialIdx];
        resetRig = true;
        bManaged = true;
    }

    if (event.GetId() == wxID_RIG_PORT) {
        if (rigPortDialog == nullptr) {
            rigPortDialog = new PortSelectorDialog(this, wxID_ANY, rigPort);
            rigPortDialog->ShowModal();
        }
    }

    if (event.GetId() == wxID_RIG_TOGGLE) {
        resetRig = false;
        if (!wxGetApp().rigIsActive()) {
            enableRig();
        } else {
            disableRig();
        }

        bManaged = true;
    }

    if (event.GetId() == wxID_RIG_SDR_IF) {
        if (devInfo != nullptr) {
            std::string deviceId = devInfo->getDeviceId();
            DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(deviceId);
            long long freqRigIF = wxGetNumberFromUser("Rig SDR-IF Frequency", "Frequency (Hz)", "Frequency",
                                                      devConfig->getRigIF(rigModel), 0, 2000000000);
            if (freqRigIF != -1) {
                rigSDRIF = freqRigIF;
                devConfig->setRigIF(rigModel, rigSDRIF);
            }
            if (rigSDRIF && wxGetApp().rigIsActive()) {
                wxGetApp().lockFrequency(rigSDRIF);
            } else {
                wxGetApp().unlockFrequency();
            }
        }
        bManaged = true;
    }

    if (event.GetId() == wxID_RIG_CONTROL) {
        if (wxGetApp().rigIsActive()) {
            RigThread *rt = wxGetApp().getRigThread();
            rt->setControlMode(!rt->getControlMode());
            rigControlMenuItem->Check(rt->getControlMode());
            wxGetApp().getConfig()->setRigControlMode(rt->getControlMode());
        } else {
            wxGetApp().getConfig()->setRigControlMode(rigControlMenuItem->IsChecked());
        }
        bManaged = true;
    }

    if (event.GetId() == wxID_RIG_FOLLOW) {
        if (wxGetApp().rigIsActive()) {
            RigThread *rt = wxGetApp().getRigThread();
            rt->setFollowMode(!rt->getFollowMode());
            rigFollowMenuItem->Check(rt->getFollowMode());
            wxGetApp().getConfig()->setRigFollowMode(rt->getFollowMode());
        } else {
            wxGetApp().getConfig()->setRigFollowMode(rigFollowMenuItem->IsChecked());
        }

        bManaged = true;
    }

    if (event.GetId() == wxID_RIG_CENTERLOCK) {
        if (wxGetApp().rigIsActive()) {
            RigThread *rt = wxGetApp().getRigThread();
            rt->setCenterLock(!rt->getCenterLock());
            rigCenterLockMenuItem->Check(rt->getCenterLock());
            wxGetApp().getConfig()->setRigCenterLock(rt->getCenterLock());
        } else {
            wxGetApp().getConfig()->setRigCenterLock(rigCenterLockMenuItem->IsChecked());
        }

        bManaged = true;
    }

    if (event.GetId() == wxID_RIG_FOLLOW_MODEM) {
        if (wxGetApp().rigIsActive()) {
            RigThread *rt = wxGetApp().getRigThread();
            rt->setFollowModem(!rt->getFollowModem());
            rigFollowModemMenuItem->Check(rt->getFollowModem());
            wxGetApp().getConfig()->setRigFollowModem(rt->getFollowModem());
        } else {
            wxGetApp().getConfig()->setRigFollowModem(rigFollowModemMenuItem->IsChecked());
        }

        bManaged = true;
    }

    if (wxGetApp().rigIsActive() && resetRig) {
        wxGetApp().stopRig();
        wxGetApp().initRig(rigModel, rigPort, rigSerialRate);
    }
#endif

    return bManaged;
}

bool AppFrame::actionOnMenuClose(wxCommandEvent &event) {
    if (event.GetId() == wxID_CLOSE || event.GetId() == wxID_EXIT) {
        Close(false);
        return true;
    }
    return false;
}

bool AppFrame::actionOnMenuSetPPM(wxCommandEvent &event) {
    if (event.GetId() == wxID_SET_PPM) {
        long ofs = wxGetNumberFromUser(
                "Frequency correction for device in PPM.\ni.e. -51 for -51 PPM\n\nNote: you can adjust PPM interactively\nby holding ALT over the frequency tuning bar.\n",
                "Parts per million (PPM)",
                "Frequency Correction", wxGetApp().getPPM(), -1000, 1000, this);
        wxGetApp().setPPM(ofs);

        settingsMenuItems[wxID_SET_PPM]->SetItemLabel(
                getSettingsLabel("Device PPM", to_string(wxGetApp().getPPM()), "ppm"));

        return true;
    }
    return false;
}

bool AppFrame::actionOnMenuSDRDevices(wxCommandEvent &event) {
    if (event.GetId() == wxID_SDR_DEVICES) {
        wxGetApp().deviceSelector();
        return true;
    }
    return false;
}

bool AppFrame::actionOnMenuDBOffset(wxCommandEvent &event) {
    if (event.GetId() == wxID_SET_DB_OFFSET) {
        long ofs = wxGetNumberFromUser("Shift the displayed RF power level by this amount.\ni.e. -30 for -30 dB",
                                       "Decibels (dB)",
                                       "Power Level Offset", wxGetApp().getConfig()->getDBOffset(), -1000, 1000, this);
        if (ofs != -1) {
            wxGetApp().getConfig()->setDBOffset(ofs);
            settingsMenuItems[wxID_SET_DB_OFFSET]->SetItemLabel(
                    getSettingsLabel("Power Level Offset", to_string(wxGetApp().getConfig()->getDBOffset()), "dB"));
        }
        return true;
    }
    return false;
}

bool AppFrame::actionOnMenuFreqOffset(wxCommandEvent &event) {
    if (event.GetId() == wxID_SET_FREQ_OFFSET) {
        //enter in KHz to accomodate > 2GHz shifts for down/upconverters on 32 bit platforms.
        long ofs = wxGetNumberFromUser(
                "Shift the displayed frequency by this amount of KHz.\ni.e. -125000 for -125 MHz", "Frequency (KHz)",
                "Frequency Offset", (long long) (wxGetApp().getOffset() / 1000.0), -2000000000, 2000000000, this);
        if (ofs != -1) {
            wxGetApp().setOffset((long long) ofs * 1000);

            settingsMenuItems[wxID_SET_FREQ_OFFSET]->SetItemLabel(
                    getSettingsLabel("Frequency Offset", to_string(wxGetApp().getOffset() / 1000), "KHz"));
        }
        return true;
    }
    return false;
}

bool AppFrame::actionOnMenuIQSwap(wxCommandEvent &event) {
    if (event.GetId() == wxID_SET_IQSWAP) {
        wxGetApp().getSDRThread()->setIQSwap(!wxGetApp().getSDRThread()->getIQSwap());
        return true;
    }
    return false;
}

bool AppFrame::actionOnMenuTips(wxCommandEvent &event) {
    if (event.GetId() == wxID_SET_TIPS) {
        wxGetApp().getConfig()->setShowTips(!wxGetApp().getConfig()->getShowTips());
        return true;
    }
    return false;
}

bool AppFrame::actionOnMenuPerformance(wxCommandEvent &event) {
    if (event.GetId() >= wxID_PERF_BASE && event.GetId() <= wxID_PERF_BASE + (int) AppConfig::PERF_HIGH) {

        int perfEnumAsInt = event.GetId() - wxID_PERF_BASE;
        AppConfig::PerfModeEnum perfEnumSet = AppConfig::PERF_NORMAL;

        if (perfEnumAsInt == (int) AppConfig::PERF_HIGH) {
            perfEnumSet = AppConfig::PERF_HIGH;

        } else if (perfEnumAsInt == (int) AppConfig::PERF_LOW) {
            perfEnumSet = AppConfig::PERF_LOW;
        }

        wxGetApp().getConfig()->setPerfMode(perfEnumSet);

        //update Channelizer mode:
        if (perfEnumSet == AppConfig::PERF_HIGH) {
            wxGetApp().setChannelizerType(SDRPostPFBCH2);
        } else {
            wxGetApp().setChannelizerType(SDRPostPFBCH);
        }

        //update UI
        wxMenuItem *selectedPerfModeItem = performanceMenuItems[event.GetId()];
        performanceMenuItems[wxID_PERF_CURRENT]->SetItemLabel(
                getSettingsLabel("CPU usage", selectedPerfModeItem->GetItemLabel().ToStdString()));

        return true;
    }
    return false;
}

bool AppFrame::actionOnMenuSDRStartStop(wxCommandEvent &event) {
    if (event.GetId() == wxID_SDR_START_STOP) {
        if (!wxGetApp().getSDRThread()->isTerminated()) {
            wxGetApp().stopDevice(true, 2000);
        } else {
            SDRDeviceInfo *dev = wxGetApp().getDevice();
            if (dev != nullptr) {
                wxGetApp().setDevice(dev, 0);
            }
        }
        return true;
    }
    return false;
}

void AppFrame::OnClose(wxCloseEvent& event) {
    wxGetApp().closeDeviceSelector();
    if (aboutDlg) {
        aboutDlg->Destroy();
    }

    if (wxGetApp().getDemodSpectrumProcessor()) {
        wxGetApp().getDemodSpectrumProcessor()->removeOutput(demodSpectrumCanvas->getVisualDataQueue());
        wxGetApp().getDemodSpectrumProcessor()->removeOutput(demodWaterfallCanvas->getVisualDataQueue());
    }
    wxGetApp().getSpectrumProcessor()->removeOutput(spectrumCanvas->getVisualDataQueue());

    if (saveDisabled) {
        event.Skip();
        return;
    }
    
#ifdef __APPLE__
    if (this->GetPosition().y > 0) {
        wxGetApp().getConfig()->setWindow(this->GetPosition(), this->GetClientSize());
        wxGetApp().getConfig()->setWindowMaximized(this->IsMaximized());
    }
#else
    wxGetApp().getConfig()->setWindow(this->GetPosition(), this->GetClientSize());
    wxGetApp().getConfig()->setWindowMaximized(this->IsMaximized());
#endif
    wxGetApp().getConfig()->setTheme(ThemeMgr::mgr.getTheme());
    wxGetApp().getConfig()->setFontScale(GLFont::getScale());
    wxGetApp().getConfig()->setSnap(wxGetApp().getFrequencySnap());
    wxGetApp().getConfig()->setCenterFreq(wxGetApp().getFrequency());
    wxGetApp().getConfig()->setSpectrumAvgSpeed(wxGetApp().getSpectrumProcessor()->getFFTAverageRate());
    wxGetApp().getConfig()->setWaterfallLinesPerSec(waterfallDataThread->getLinesPerSecond());
    wxGetApp().getConfig()->setManualDevices(SDREnumerator::getManuals());
    wxGetApp().getConfig()->setModemPropsCollapsed(modemProps->isCollapsed());
    wxGetApp().getConfig()->setMainSplit(mainSplitter->GetSashPosition());
    wxGetApp().getConfig()->setVisSplit(mainVisSplitter->GetSashPosition());
    if (!hideBookmarksItem->IsChecked()) wxGetApp().getConfig()->setBookmarkSplit(bookmarkSplitter->GetSashPosition());
    wxGetApp().getConfig()->setBookmarksVisible(!hideBookmarksItem->IsChecked());
#ifdef USE_HAMLIB
    wxGetApp().getConfig()->setRigEnabled(rigEnableMenuItem->IsChecked());
    wxGetApp().getConfig()->setRigModel(rigModel);
    wxGetApp().getConfig()->setRigRate(rigSerialRate);
    wxGetApp().getConfig()->setRigPort(rigPort);
    wxGetApp().getConfig()->setRigFollowMode(rigFollowMenuItem->IsChecked());
    wxGetApp().getConfig()->setRigControlMode(rigControlMenuItem->IsChecked());
    wxGetApp().getConfig()->setRigCenterLock(rigCenterLockMenuItem->IsChecked());
    wxGetApp().getConfig()->setRigFollowModem(rigFollowModemMenuItem->IsChecked());
#endif
    wxGetApp().getConfig()->save();
    wxGetApp().getBookmarkMgr().saveToFile("bookmarks.xml");
    event.Skip();
}

void AppFrame::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
    new AppFrame();
}

void AppFrame::OnIdle(wxIdleEvent &event) {

    handleUpdateDeviceParams();

    handleTXAntennaChange();

    DemodulatorInstancePtr demod = wxGetApp().getDemodMgr().getCurrentModem();

    if (demod && demod->isModemInitialized()) {
        handleCurrentModem();
    } else if (demod) {
        // Wait state for current demodulator modem to activate..
    } else {
        handleModeSelector();
        handleGainMeter();
        handleDemodWaterfallSpectrum();
        handleSpectrumWaterfall();
        handleMuteButton();
    }

    handleScopeProcessor();
    handleScopeSpectrumProcessors();
    handleModemProperties();
    handlePeakHold();

#if USE_HAMLIB
    handleRigMenu();
#endif

#ifdef _WIN32
    if (scopeCanvas && scopeCanvas->HasFocus()) {
        waterfallCanvas->SetFocus();
    }
#endif

    if (!this->IsActive()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    } else {
        if (wxGetApp().getConfig()->getPerfMode() == AppConfig::PERF_LOW) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    event.RequestMore();
}

void AppFrame::handleTXAntennaChange() {//Refresh the current TX antenna on, if any:
    if ((antennaMenuItems.find(wxID_ANTENNA_CURRENT_TX) != antennaMenuItems.end()) && devInfo) {
        string actualTxAntenna = devInfo->getAntennaName(SOAPY_SDR_TX, 0);

        if (currentTXantennaName != actualTxAntenna) {
            currentTXantennaName = actualTxAntenna;
            antennaMenuItems[wxID_ANTENNA_CURRENT_TX]->SetItemLabel(
                    getSettingsLabel("TX Antenna", currentTXantennaName));
        }
    }
}

#if USE_HAMLIB
void AppFrame::handleRigMenu() {
    if (rigEnableMenuItem->IsChecked()) {
        if (!wxGetApp().rigIsActive()) {
            rigEnableMenuItem->Check(false);
            wxGetApp().getConfig()->setRigEnabled(false);
        }
    }
}
#endif

void AppFrame::handlePeakHold() {
    int peakHoldMode = peakHoldButton->getSelection();
    if (peakHoldButton->modeChanged()) {
        wxGetApp().getSpectrumProcessor()->setPeakHold(peakHoldMode == 1);

        //make the peak hold act on the current dmod also, like a zoomed-in version.
        if (wxGetApp().getDemodSpectrumProcessor()) {
            wxGetApp().getDemodSpectrumProcessor()->setPeakHold(peakHoldMode == 1);
        }
        peakHoldButton->clearModeChanged();
    }
}

void AppFrame::handleModemProperties() {
    DemodulatorInstancePtr demod = wxGetApp().getDemodMgr().getCurrentModem();

    if (modemPropertiesUpdated.load() && demod && demod->isModemInitialized()) {

        //reset notification flag
        modemPropertiesUpdated.store(false);

        modemProps->initProperties(demod->getModemArgs(), demod);
        modemProps->updateTheme();
        demodTray->Layout();
        modemProps->fitColumns();
#if ENABLE_DIGITAL_LAB
        if (demod->getModemType() == "digital") {
            ModemDigitalOutputConsole *outp = (ModemDigitalOutputConsole *) demod->getOutput();
            if (!outp->getDialog()) {
                outp->setTitle(demod->getDemodulatorType() + ": " + frequencyToStr(demod->getFrequency()));
                outp->setDialog(new DigitalConsole(this, outp));
            }
            demod->showOutput();
        }
#endif
    } else if (!demod && modemPropertiesUpdated.load()) {
        ModemArgInfoList dummyInfo;
        modemProps->initProperties(dummyInfo, nullptr);
        modemProps->updateTheme();
        demodTray->Layout();
    }

    if (modemProps->IsShown() && modemProps->isCollapsed() && modemProps->GetMinWidth() > 22) {
        modemProps->SetMinSize(wxSize(APPFRAME_MODEMPROPS_MINSIZE, -1));
        modemProps->SetMaxSize(wxSize(APPFRAME_MODEMPROPS_MINSIZE, -1));
        demodTray->Layout();
        modemProps->fitColumns();
    } else if (modemProps->IsShown() && !modemProps->isCollapsed() && modemProps->GetMinWidth() < 200) {
        modemProps->SetMinSize(wxSize(APPFRAME_MODEMPROPS_MAXSIZE, -1));
        modemProps->SetMaxSize(wxSize(APPFRAME_MODEMPROPS_MAXSIZE, -1));
        demodTray->Layout();
        modemProps->fitColumns();
    }
}

void AppFrame::handleScopeSpectrumProcessors() {
    SpectrumVisualProcessor *proc = wxGetApp().getSpectrumProcessor();

    if (spectrumAvgMeter->inputChanged()) {
        float val = spectrumAvgMeter->getInputValue();
        if (val < 0.01) {
            val = 0.01f;
        }
        if (val > 0.99) {
            val = 0.99f;
        }
        spectrumAvgMeter->setLevel(val);
        proc->setFFTAverageRate(val);

        GetStatusBar()->SetStatusText(
                wxString::Format(wxT("Spectrum averaging speed changed to %0.2f%%."), val * 100.0));
    }

    SpectrumVisualProcessor *dproc = wxGetApp().getDemodSpectrumProcessor();

    if (dproc) {
        dproc->setView(demodWaterfallCanvas->getViewState(), demodWaterfallCanvas->getCenterFrequency(),
                       demodWaterfallCanvas->getBandwidth());
    }

    SpectrumVisualProcessor *wproc = waterfallDataThread->getProcessor();

    if (waterfallSpeedMeter->inputChanged()) {
        float val = waterfallSpeedMeter->getInputValue();
        waterfallSpeedMeter->setLevel(val);
        waterfallDataThread->setLinesPerSecond((int) ceil(val * val));
        waterfallCanvas->setLinesPerSecond((int) ceil(val * val));
        GetStatusBar()->SetStatusText(
                wxString::Format(wxT("Waterfall max speed changed to %d lines per second."), (int) ceil(val * val)));
    }

    wproc->setView(waterfallCanvas->getViewState(), waterfallCanvas->getCenterFrequency(),
                   waterfallCanvas->getBandwidth());

    proc->setView(wproc->isView(), wproc->getCenterFrequency(), wproc->getBandwidth());
}

void AppFrame::handleScopeProcessor() {
    if (scopeCanvas) {
        scopeCanvas->setPPMMode(demodTuner->isAltDown());

        wxGetApp().getScopeProcessor()->setScopeEnabled(scopeCanvas->scopeVisible());
        wxGetApp().getScopeProcessor()->setSpectrumEnabled(scopeCanvas->spectrumVisible());
        wxGetApp().getAudioVisualQueue()->set_max_num_items(
                (scopeCanvas->scopeVisible() ? 1 : 0) + (scopeCanvas->spectrumVisible() ? 1 : 0));

        wxGetApp().getScopeProcessor()->run();
    }
}

void AppFrame::handleMuteButton() {
    if (demodMuteButton->modeChanged()) {
        int muteMode = demodMuteButton->getSelection();
        if (muteMode == -1) {
            wxGetApp().getDemodMgr().setLastMuted(false);
        } else if (muteMode == 1) {
            wxGetApp().getDemodMgr().setLastMuted(true);
        }
        demodMuteButton->clearModeChanged();
    }
}

void AppFrame::handleSpectrumWaterfall() {
    if (spectrumCanvas->getViewState() &&
        abs(wxGetApp().getFrequency() - spectrumCanvas->getCenterFrequency()) > (wxGetApp().getSampleRate() / 2)) {
        spectrumCanvas->setCenterFrequency(wxGetApp().getFrequency());
        waterfallCanvas->setCenterFrequency(wxGetApp().getFrequency());
    }
}

void AppFrame::handleDemodWaterfallSpectrum() {
    if (demodWaterfallCanvas && wxGetApp().getFrequency() != demodWaterfallCanvas->getCenterFrequency()) {
        demodWaterfallCanvas->setCenterFrequency(wxGetApp().getFrequency());
        if (demodSpectrumCanvas) {
            demodSpectrumCanvas->setCenterFrequency(wxGetApp().getFrequency());
        }
    }
}

void AppFrame::handleGainMeter() {
    DemodulatorMgr *mgr = &wxGetApp().getDemodMgr();

    demodGainMeter->setLevel(mgr->getLastGain());
    if (demodSignalMeter->inputChanged()) {
        mgr->setLastSquelchLevel(demodSignalMeter->getInputValue());
    }
    if (demodGainMeter->inputChanged()) {
        mgr->setLastGain(demodGainMeter->getInputValue());
        demodGainMeter->setLevel(demodGainMeter->getInputValue());
    }
}

void AppFrame::handleModeSelector() {
    DemodulatorMgr *mgr = &wxGetApp().getDemodMgr();

    string dSelection = demodModeSelector->getSelectionLabel();
#ifdef ENABLE_DIGITAL_LAB
    string dSelectionadv = demodModeSelectorAdv->getSelectionLabel();

    // basic demodulators
    if (dSelection != "" && dSelection != mgr->getLastDemodulatorType()) {
        mgr->setLastDemodulatorType(dSelection);
        mgr->setLastBandwidth(Modem::getModemDefaultSampleRate(dSelection));
        demodTuner->setHalfBand(dSelection == "USB" || dSelection == "LSB");
        demodModeSelectorAdv->setSelection(-1);
    }
        // advanced demodulators
    else if (dSelectionadv != "" && dSelectionadv != mgr->getLastDemodulatorType()) {
        mgr->setLastDemodulatorType(dSelectionadv);
        mgr->setLastBandwidth(Modem::getModemDefaultSampleRate(dSelectionadv));
        demodTuner->setHalfBand(false);
        demodModeSelector->setSelection(-1);
    }
#else
    // basic demodulators
    if (dSelection != "" && dSelection != mgr->getLastDemodulatorType()) {
        mgr->setLastDemodulatorType(dSelection);
        mgr->setLastBandwidth(Modem::getModemDefaultSampleRate(dSelection));
        demodTuner->setHalfBand(dSelection=="USB" || dSelection=="LSB");
    }
#endif
}

void AppFrame::handleCurrentModem() {

    DemodulatorInstancePtr demod = wxGetApp().getDemodMgr().getCurrentModem();

    if (demod->isTracking()) {
        if (spectrumCanvas->getViewState()) {
            long long diff =
                    abs(demod->getFrequency() - spectrumCanvas->getCenterFrequency()) + (demod->getBandwidth() / 2) +
                    (demod->getBandwidth() / 4);

            if (diff > spectrumCanvas->getBandwidth() / 2) {
                if (demod->getBandwidth() > (int) spectrumCanvas->getBandwidth()) {
                    diff = abs(demod->getFrequency() - spectrumCanvas->getCenterFrequency());
                } else {
                    diff = diff - spectrumCanvas->getBandwidth() / 2;
                }
                spectrumCanvas->moveCenterFrequency(
                        (demod->getFrequency() < spectrumCanvas->getCenterFrequency()) ? diff : -diff);
                demod->setTracking(false);
            }
        } else {
            demod->setTracking(false);
        }
    }

    if (demod->getBandwidth() != wxGetApp().getDemodMgr().getLastBandwidth()) {
        wxGetApp().getDemodMgr().setLastBandwidth(demod->getBandwidth());
    }

    if (demod.get() != activeDemodulator) {
        demodSignalMeter->setInputValue(demod->getSquelchLevel());
        demodGainMeter->setInputValue(demod->getGain());
        wxGetApp().getDemodMgr().setLastGain(demod->getGain());
        int outputDevice = demod->getOutputDevice();
        if (scopeCanvas) {
            auto outputDevices = wxGetApp().getDemodMgr().getOutputDevices();
            scopeCanvas->setDeviceName(outputDevices[outputDevice].name);
        }
//            outputDeviceMenuItems[outputDevice]->Check(true);
        string dType = demod->getDemodulatorType();
        demodModeSelector->setSelection(dType);
#ifdef ENABLE_DIGITAL_LAB
        demodModeSelectorAdv->setSelection(dType);
#endif
        deltaLockButton->setSelection(demod->isDeltaLock() ? 1 : -1);
        demodMuteButton->setSelection(demod->isMuted() ? 1 : -1);
        modemPropertiesUpdated.store(true);
        demodTuner->setHalfBand(dType == "USB" || dType == "LSB");
    }
    if (!demodWaterfallCanvas || demodWaterfallCanvas->getDragState() == WaterfallCanvas::WF_DRAG_NONE) {
        long long centerFreq = demod->getFrequency();
        unsigned int demodBw = (unsigned int) ceil((float) demod->getBandwidth() * 2.25);

        if (demod->getDemodulatorType() == "USB") {
            demodBw /= 2;
            centerFreq += demod->getBandwidth() / 4;
        }

        if (demod->getDemodulatorType() == "LSB") {
            demodBw /= 2;
            centerFreq -= demod->getBandwidth() / 4;
        }

        if (demodBw > wxGetApp().getSampleRate() / 2) {
            demodBw = wxGetApp().getSampleRate() / 2;
        }
        if (demodBw < 20000) {
            demodBw = 20000;
        }

        if (demodWaterfallCanvas && centerFreq != demodWaterfallCanvas->getCenterFrequency()) {
            demodWaterfallCanvas->setCenterFrequency(centerFreq);
            demodSpectrumCanvas->setCenterFrequency(centerFreq);
        }

        string dSelection = demodModeSelector->getSelectionLabel();
#ifdef ENABLE_DIGITAL_LAB
        string dSelectionadv = demodModeSelectorAdv->getSelectionLabel();

        // basic demodulators
        if (dSelection != "" && dSelection != demod->getDemodulatorType()) {
            demod->setDemodulatorType(dSelection);
            demodTuner->setHalfBand(dSelection == "USB" || dSelection == "LSB");
            demodModeSelectorAdv->setSelection(-1);
        }
            // advanced demodulators
        else if (dSelectionadv != "" && dSelectionadv != demod->getDemodulatorType()) {
            demod->setDemodulatorType(dSelectionadv);
            demodTuner->setHalfBand(false);
            demodModeSelector->setSelection(-1);
        }
#else
        // basic demodulators
        if (dSelection != "" && dSelection != demod->getDemodulatorType()) {
            demod->setDemodulatorType(dSelection);
            demodTuner->setHalfBand(dSelection=="USB" || dSelection=="LSB");
        }
#endif

        int muteMode = demodMuteButton->getSelection();
        if (demodMuteButton->modeChanged()) {
            if (demod->isMuted() && muteMode == -1) {
                demod->setMuted(false);
            } else if (!demod->isMuted() && muteMode == 1) {
                demod->setMuted(true);
            }
            wxGetApp().getDemodMgr().setLastMuted(demod->isMuted());
            demodMuteButton->clearModeChanged();
        } else {
            if (demod->isMuted() && muteMode == -1) {
                demodMuteButton->setSelection(1);
                wxGetApp().getDemodMgr().setLastMuted(demod->isMuted());
                demodMuteButton->Refresh();
            } else if (!demod->isMuted() && muteMode == 1) {
                demodMuteButton->setSelection(-1);
                wxGetApp().getDemodMgr().setLastMuted(demod->isMuted());
                demodMuteButton->Refresh();
            }
        }

        int deltaMode = deltaLockButton->getSelection();
        if (deltaLockButton->modeChanged()) {
            if (demod->isDeltaLock() && deltaMode == -1) {
                demod->setDeltaLock(false);
            } else if (!demod->isDeltaLock() && deltaMode == 1) {
                demod->setDeltaLockOfs(demod->getFrequency() - wxGetApp().getFrequency());
                demod->setDeltaLock(true);
            }
            wxGetApp().getDemodMgr().setLastDeltaLock(demod->isDeltaLock());
            deltaLockButton->clearModeChanged();
        } else {
            if (demod->isDeltaLock() && deltaMode == -1) {
                deltaLockButton->setSelection(1);
                wxGetApp().getDemodMgr().setLastDeltaLock(true);
                deltaLockButton->Refresh();
            } else if (!demod->isDeltaLock() && deltaMode == 1) {
                deltaLockButton->setSelection(-1);
                wxGetApp().getDemodMgr().setLastDeltaLock(false);
                deltaLockButton->Refresh();
            }
        }

        int soloMode = soloModeButton->getSelection();
        if (soloModeButton->modeChanged()) {
            if (soloMode == 1) {
                wxGetApp().setSoloMode(true);
            } else {
                wxGetApp().setSoloMode(false);
            }
            soloModeButton->clearModeChanged();
        } else {
            if (wxGetApp().getSoloMode() != (soloMode == 1)) {
                soloModeButton->setSelection(wxGetApp().getSoloMode() ? 1 : -1);
                soloModeButton->Refresh();
            }
        }

        if (demodWaterfallCanvas) {
            demodWaterfallCanvas->setBandwidth(demodBw);
            demodSpectrumCanvas->setBandwidth(demodBw);
        }
    }

    demodSignalMeter->setLevel(demod->getSignalLevel());
    demodSignalMeter->setMin(demod->getSignalFloor());
    demodSignalMeter->setMax(demod->getSignalCeil());

    demodGainMeter->setLevel(demod->getGain());
    if (demodSignalMeter->inputChanged()) {
        demod->setSquelchLevel(demodSignalMeter->getInputValue());
    }
    if (demodGainMeter->inputChanged()) {
        demod->setGain(demodGainMeter->getInputValue());
        demodGainMeter->setLevel(demodGainMeter->getInputValue());
    }
    activeDemodulator = demod.get();
}


void AppFrame::OnDoubleClickSash(wxSplitterEvent& event)
{
    wxWindow *a, *b;
    wxSplitterWindow *w = NULL;
    float g = 0.5;

    if (event.GetId() == wxID_MAIN_SPLITTER) {
        w = mainSplitter;
        g = 10.0f/37.0f;
    } else if (event.GetId() == wxID_VIS_SPLITTER) {
        w = mainVisSplitter;
        g = 6.0f/25.0f;
    }

    if (w != NULL) {
        a = w->GetWindow1();
        b = w->GetWindow2();
        w->Unsplit();
        w->SetSashGravity(g);
        wxSize s = w->GetSize();
        
        w->SplitHorizontally(a, b, int(float(s.GetHeight()) * g));
    }
    
    event.Veto();
}

void AppFrame::OnUnSplit(wxSplitterEvent& event)
{
    event.Veto();
}

void AppFrame::OnAboutDialogClose(wxCommandEvent& /* event */) {
    aboutDlg->Destroy();
    aboutDlg = nullptr;
}

void AppFrame::saveSession(std::string fileName) {
    wxGetApp().getSessionMgr().saveSession(fileName);

    currentSessionFile = fileName;
    std::string filePart = fileName.substr(fileName.find_last_of(filePathSeparator) + 1);
    GetStatusBar()->SetStatusText(wxString::Format(wxT("Saved session: %s"), currentSessionFile.c_str()));
    SetTitle(wxString::Format(wxT("%s: %s"), CUBICSDR_TITLE, filePart.c_str()));
}

bool AppFrame::loadSession(std::string fileName) {
    bool result = wxGetApp().getSessionMgr().loadSession(fileName);

    int sample_rate = wxGetApp().getSampleRate();

    //scan the available sample rates and see if it matches a predifined one
    int menuIndex = -1;
    for (auto discreteRate : sampleRates) {
        if (discreteRate == sample_rate) {
            menuIndex++;
            //activate Bandwidth Menu entry matching this predefined sample_rate.
            sampleRateMenuItems[wxID_BANDWIDTH_BASE + menuIndex]->Check(true);
            break;
        }
    } //end for
    //this is a manual entry
    if (menuIndex == -1) {
        manualSampleRate = sample_rate;
        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->Enable(true);
        // Apply the manual value, activate the menu entry

        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->SetItemLabel(wxString("Manual Entry :  ") + frequencyToStr(sample_rate));
        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->Check(true);
    }

    deviceChanged.store(true);

    currentSessionFile = fileName;

    std::string filePart = fileName.substr(fileName.find_last_of(filePathSeparator) + 1);

    GetStatusBar()->SetStatusText(wxString::Format(wxT("Loaded session file: %s"), currentSessionFile.c_str()));
    SetTitle(wxString::Format(wxT("%s: %s"), CUBICSDR_TITLE, filePart.c_str()));

    wxGetApp().getBookmarkMgr().updateActiveList();

    return result;
}

FFTVisualDataThread *AppFrame::getWaterfallDataThread() {
    return waterfallDataThread;
}

WaterfallCanvas *AppFrame::getWaterfallCanvas() {
    return waterfallCanvas;
}

SpectrumCanvas *AppFrame::getSpectrumCanvas() {
    return spectrumCanvas;
}
void AppFrame::notifyUpdateModemProperties() {
   
    modemPropertiesUpdated.store(true);
}

void AppFrame::setMainWaterfallFFTSize(int fftSize) {
    wxGetApp().getSpectrumProcessor()->setFFTSize(fftSize);
    spectrumCanvas->setFFTSize(fftSize);
    waterfallDataThread->getProcessor()->setFFTSize(fftSize);
    waterfallCanvas->setFFTSize(fftSize);
}

void AppFrame::setScopeDeviceName(std::string deviceName) {
    if (scopeCanvas) {
        scopeCanvas->setDeviceName(deviceName);
    }
}


void AppFrame::refreshGainUI() {
    gainCanvas->updateGainUI();
    gainCanvas->Refresh();
}

bool AppFrame::isUserDemodBusy() {
    return (modemProps && modemProps->isMouseInView())
        || (waterfallCanvas->isMouseInView() && waterfallCanvas->isMouseDown())
        || (demodWaterfallCanvas && demodWaterfallCanvas->isMouseInView() && demodWaterfallCanvas->isMouseDown())
        || (wxGetApp().getDemodMgr().getCurrentModem() &&
            wxGetApp().getDemodMgr().getActiveContextModem() &&
            wxGetApp().getDemodMgr().getCurrentModem() != wxGetApp().getDemodMgr().getActiveContextModem());
}

BookmarkView *AppFrame::getBookmarkView() {
    return bookmarkView;
}

void AppFrame::disableSave(bool state) {
    saveDisabled = state;
}


#ifdef _WIN32
bool AppFrame::canFocus() {
	return (!wxGetApp().isDeviceSelectorOpen() && (!modemProps || !modemProps->isMouseInView()));
}
#endif

FrequencyDialog::FrequencyDialogTarget AppFrame::getFrequencyDialogTarget() {
    FrequencyDialog::FrequencyDialogTarget target = FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_DEFAULT;
    
    if (waterfallSpeedMeter->getMouseTracker()->mouseInView()) {
        target = FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_WATERFALL_LPS;
    }
    else if (spectrumAvgMeter->getMouseTracker()->mouseInView()) {
        target = FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_SPECTRUM_AVG;
    }
    else if (demodTuner->getMouseTracker()->mouseInView()) {
        switch (demodTuner->getHoverState()) {
            case TuningCanvas::ActiveState::TUNING_HOVER_BW:
                target = FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_BANDWIDTH;
                break;
            case TuningCanvas::ActiveState::TUNING_HOVER_FREQ:
                target = FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_FREQ;
                break;
            case TuningCanvas::ActiveState::TUNING_HOVER_CENTER:
            default:
                target = FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_DEFAULT;
                break;
                
        }
    }
    else if (gainCanvas->getMouseTracker()->mouseInView()) {
        target = FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_GAIN;
    }
    return target;
}

void AppFrame::gkNudgeLeft(DemodulatorInstancePtr demod, int snap) {
    if (demod) {
        demod->setFrequency(demod->getFrequency()-snap);
        demod->updateLabel(demod->getFrequency());
    }
}

void AppFrame::gkNudgeRight(DemodulatorInstancePtr demod, int snap) {
    if (demod) {
        demod->setFrequency(demod->getFrequency()+snap);
        demod->updateLabel(demod->getFrequency());
    }
}

int AppFrame::OnGlobalKeyDown(wxKeyEvent &event) {
    if (!this->IsActive()) {
        return -1;
    }
    
#ifdef USE_HAMLIB
    if (rigPortDialog != nullptr) {
        return -1;
    }
#endif
    
    if (modemProps && (modemProps->HasFocus() || modemProps->isMouseInView())) {
        return -1;
    }
    
    if (bookmarkView && bookmarkView->isMouseInView()) {
        return -1;
    }
    
    DemodulatorInstancePtr demod = nullptr;
     
    DemodulatorInstancePtr lastDemod = wxGetApp().getDemodMgr().getCurrentModem();
    
    int snap = wxGetApp().getFrequencySnap();
    
    if (event.ControlDown()) {
        return 1;
    }
    
    if (event.ShiftDown()) {
        if (snap != 1) {
            snap /= 2;
        }
    }
    
    #ifdef wxHAS_RAW_KEY_CODES
    switch (event.GetRawKeyCode()) {
        case 30:
            gkNudgeRight(lastDemod, snap);
            return 1;
        case 33:
            gkNudgeLeft(lastDemod, snap);
            return 1;
    }
    #endif
    
    
    switch (event.GetKeyCode()) {
        case WXK_UP:
        case WXK_NUMPAD_UP:
        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
        case WXK_LEFT:
        case WXK_NUMPAD_LEFT:
        case WXK_RIGHT:
        case WXK_NUMPAD_RIGHT:
            waterfallCanvas->OnKeyDown(event);  // TODO: Move the stuff from there to here
            return 1;
        case 'V':
            return 1;
        case ']':
            gkNudgeRight(lastDemod, snap);
            return 1;
        case '[':
            gkNudgeLeft(lastDemod, snap);
            return 1;
        case 'A':
        case 'F':
        case 'L':
        case 'U':
        case 'S':
        case 'P':
        case 'M':
        case 'R':
            return 1;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            wxGetApp().showFrequencyInput(getFrequencyDialogTarget(), std::to_string(event.GetKeyCode() - '0'));
            return 1;
            break;
        case WXK_TAB:
            lastDemod = wxGetApp().getDemodMgr().getCurrentModem();
            if (!lastDemod) {
                break;
            }
            if (event.ShiftDown()) {
                demod = wxGetApp().getDemodMgr().getPreviousDemodulator(lastDemod);
            } else {
                demod = wxGetApp().getDemodMgr().getNextDemodulator(lastDemod);
            }
            if (demod) {
                wxGetApp().getDemodMgr().setActiveDemodulator(nullptr);
                wxGetApp().getDemodMgr().setActiveDemodulator(demod, false);
            }
            return 1;
        default:
            break;
    }
    
    //Re-dispatch the key events if the mouse cursor is within a given
    //widget region, effectively activating its specific key shortcuts,
    //which else are overriden by this global key handler.
    if (demodTuner->getMouseTracker()->mouseInView()) {
        demodTuner->OnKeyDown(event);
    } else if (waterfallCanvas->getMouseTracker()->mouseInView()) {
        waterfallCanvas->OnKeyDown(event);
    }
    else if (spectrumCanvas->getMouseTracker()->mouseInView()) {
        spectrumCanvas->OnKeyDown(event);
    }
    else if (scopeCanvas->getMouseTracker()->mouseInView()) {
        scopeCanvas->OnKeyDown(event);
    }

    return 1;
}

int AppFrame::OnGlobalKeyUp(wxKeyEvent &event) {
    if (!this->IsActive()) {
        return -1;
    }
    
#ifdef USE_HAMLIB
    if (rigPortDialog != nullptr) {
        return -1;
    }
#endif
    
    if (modemProps && (modemProps->HasFocus() || modemProps->isMouseInView())) {
        return -1;
    }

    if (bookmarkView && bookmarkView->isMouseInView()) {
        return -1;
    }

    if (event.ControlDown()) {
        return 1;
    }

    DemodulatorInstancePtr activeDemod = wxGetApp().getDemodMgr().getActiveContextModem();
    DemodulatorInstancePtr lastDemod = wxGetApp().getDemodMgr().getCurrentModem();
    
#ifdef wxHAS_RAW_KEY_CODES
    switch (event.GetRawKeyCode()) {
        case 30:
            return 1;
        case 33:
            return 1;
    }
#endif
    
    switch (event.GetKeyCode()) {
        case WXK_SPACE:
            if (!demodTuner->getMouseTracker()->mouseInView()) {
                wxGetApp().showFrequencyInput(getFrequencyDialogTarget());
                return 1;
            }
            break;
        case WXK_UP:
        case WXK_NUMPAD_UP:
        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
        case WXK_LEFT:
        case WXK_NUMPAD_LEFT:
        case WXK_RIGHT:
        case WXK_NUMPAD_RIGHT:
            waterfallCanvas->OnKeyUp(event);
            return 1;
        case 'V':
            if (activeDemod) {
                lastDemod = activeDemod;
            }
            if (lastDemod && lastDemod->isDeltaLock()) {
                lastDemod->setDeltaLock(false);
            } else if (lastDemod) {
                lastDemod->setDeltaLockOfs(lastDemod->getFrequency() - wxGetApp().getFrequency());
                lastDemod->setDeltaLock(true);
            }
            break;
        case 'A':
            demodModeSelector->setSelection("AM");
            return 1;
            break;
        case 'F':
            if (demodModeSelector->getSelectionLabel() == "FM") {
                demodModeSelector->setSelection("FMS");
            } else if (demodModeSelector->getSelectionLabel() == "FMS") {
                demodModeSelector->setSelection("NBFM");
            } else if (demodModeSelector->getSelectionLabel() == "NBFM") {
                demodModeSelector->setSelection("FM");
            }
            return 1;
            break;
        case 'L':
            demodModeSelector->setSelection("LSB");
            return 1;
            break;
        case 'U':
            demodModeSelector->setSelection("USB");
            return 1;
            break;
        case 'S':
            wxGetApp().setSoloMode(!wxGetApp().getSoloMode());
            return 1;
            break;
        case 'R':
            if (event.ShiftDown()) {
                toggleAllActiveDemodRecording();
            } else {
                toggleActiveDemodRecording();
            }
            break;
        case 'P':
            wxGetApp().getSpectrumProcessor()->setPeakHold(!wxGetApp().getSpectrumProcessor()->getPeakHold());
            if (wxGetApp().getDemodSpectrumProcessor()) {
                wxGetApp().getDemodSpectrumProcessor()->setPeakHold(wxGetApp().getSpectrumProcessor()->getPeakHold());
            }
            peakHoldButton->setSelection(wxGetApp().getSpectrumProcessor()->getPeakHold()?1:0);
            peakHoldButton->clearModeChanged();
            break;
        case ']':
        case '[':
            return 1;
        case 'M':
            if (activeDemod) {
                lastDemod = activeDemod;
            }
            if (lastDemod) {
                lastDemod->setMuted(!lastDemod->isMuted());
            }
            break;
        default:
            break;
    }

    //Re-dispatch the key events if the mouse cursor is within a given
    //widget region, effectively activating its specific key shortcuts,
    //which else are overriden by this global key handler.
    if (demodTuner->getMouseTracker()->mouseInView()) {
        demodTuner->OnKeyUp(event);
    }
    else if (waterfallCanvas->getMouseTracker()->mouseInView()) {
        waterfallCanvas->OnKeyUp(event);
    }
    else if (spectrumCanvas->getMouseTracker()->mouseInView()) {
        spectrumCanvas->OnKeyUp(event);
    }
    else if (scopeCanvas->getMouseTracker()->mouseInView()) {
        scopeCanvas->OnKeyUp(event);
    }
    
    // TODO: Catch key-ups outside of original target

    return 1;
}

void AppFrame::toggleActiveDemodRecording() {
    if (!wxGetApp().getConfig()->verifyRecordingPath()) {
        return;
    }
    
    DemodulatorInstancePtr activeDemod = wxGetApp().getDemodMgr().getActiveContextModem();
    
    if (activeDemod) {
        activeDemod->setRecording(!activeDemod->isRecording());
        wxGetApp().getBookmarkMgr().updateActiveList();
    }
}

void AppFrame::toggleAllActiveDemodRecording() {
    if (!wxGetApp().getConfig()->verifyRecordingPath()) {
        return;
    }

    // All demods, irrespective of their active state:
    // recording will start eventually when a demod come in range.
    auto allDemods = wxGetApp().getDemodMgr().getDemodulators();

    //by default, do a false => true for all:
    bool stateToSet = true;

    for (auto i : allDemods) {
        if (i->isRecording()) {
            stateToSet = false;
            break;
        }
    }

    for (auto i : allDemods) {
      
        i->setRecording(stateToSet);               
    }
    //this effectively refresh the BookmarkView buttons, including Recording buttons.
    wxGetApp().getBookmarkMgr().updateActiveList();
}

void AppFrame::setWaterfallLinesPerSecond(int lps) {
    waterfallSpeedMeter->setUserInputValue(sqrt(lps));
}

void AppFrame::setSpectrumAvgSpeed(double avg) {
    spectrumAvgMeter->setUserInputValue(avg);
}

void AppFrame::setViewState(long long center_freq, int bandwidth) {
    spectrumCanvas->setView(center_freq, bandwidth);
    waterfallCanvas->setView(center_freq, bandwidth);
}

void AppFrame::setViewState() {
    spectrumCanvas->setCenterFrequency(wxGetApp().getFrequency());
    waterfallCanvas->setCenterFrequency(wxGetApp().getFrequency());
    spectrumCanvas->disableView();
    waterfallCanvas->disableView();
}


long long AppFrame::getViewCenterFreq() {
    return waterfallCanvas->getCenterFrequency();

}


int AppFrame::getViewBandwidth() {
    return waterfallCanvas->getBandwidth();
}


void AppFrame::setStatusText(wxWindow* window, std::string statusText) {
    GetStatusBar()->SetStatusText(statusText);
    if (wxGetApp().getConfig()->getShowTips()) {
        if (statusText != lastToolTip) {
            wxToolTip::Enable(false);
            window->SetToolTip(statusText);
            lastToolTip = statusText;
            wxToolTip::SetDelay(1000);
            wxToolTip::Enable(true);
        }
    }
    else {
        window->SetToolTip("");
        lastToolTip = "";
    }
}

void AppFrame::setStatusText(std::string statusText, int value) {
    GetStatusBar()->SetStatusText(
        wxString::Format(statusText.c_str(), wxNumberFormatter::ToString((long)value, wxNumberFormatter::Style_WithThousandsSep)));
}

wxString AppFrame::getSettingsLabel(const std::string& settingsName,
                                    const std::string& settingsValue,
                                    const std::string& settingsSuffix) {

    size_t itemStringSize = 40;
    int justifValueSize = itemStringSize - settingsName.length() - 1;

    std::stringstream full_label;
    
    full_label << settingsName + " : ";
    full_label << std::right << std::setw(justifValueSize);

    if (settingsSuffix.empty()) {
        full_label << settingsValue;
    } else {
        full_label << settingsValue + " " + settingsSuffix;
    }

    return wxString(full_label.str());
}
