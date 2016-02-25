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
#include "AudioThread.h"
#include "CubicSDR.h"
#include "DataTree.h"
#include "ColorTheme.h"
#include "DemodulatorMgr.h"

#include <thread>

#include <wx/panel.h>

#ifdef __linux__
#include "CubicSDR.xpm"
#endif

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame)
//EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_CLOSE(AppFrame::OnClose)
EVT_MENU(wxID_ANY, AppFrame::OnMenu)
EVT_COMMAND(wxID_ANY, wxEVT_THREAD, AppFrame::OnThread)
EVT_IDLE(AppFrame::OnIdle)
EVT_SPLITTER_DCLICK(wxID_ANY, AppFrame::OnDoubleClickSash)
EVT_SPLITTER_UNSPLIT(wxID_ANY, AppFrame::OnUnSplit)
wxEND_EVENT_TABLE()

#ifdef USE_HAMLIB
#include "RigThread.h"
#endif

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, CUBICSDR_TITLE), activeDemodulator(NULL) {

#ifdef __linux__
    SetIcon(wxICON(cubicsdr));
#endif

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodVisuals = new wxBoxSizer(wxVERTICAL);
    demodTray = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *demodScopeTray = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodTunerTray = new wxBoxSizer(wxHORIZONTAL);

    int attribList[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };

    mainSplitter = new wxSplitterWindow( this, wxID_MAIN_SPLITTER, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE );
    mainSplitter->SetSashGravity(10.0/37.0);
    mainSplitter->SetMinimumPaneSize(1);

    wxPanel *demodPanel = new wxPanel(mainSplitter, wxID_ANY);
            
    gainCanvas = new GainCanvas(demodPanel, attribList);
    
    gainSizerItem = demodTray->Add(gainCanvas, 0, wxEXPAND | wxALL, 0);
    gainSizerItem->Show(false);
    gainSpacerItem = demodTray->AddSpacer(1);
    gainSpacerItem->Show(false);
            
    demodModeSelector = new ModeSelectorCanvas(demodPanel, attribList);
    demodModeSelector->addChoice("FM");
    demodModeSelector->addChoice("FMS");
    demodModeSelector->addChoice("AM");
    demodModeSelector->addChoice("LSB");
    demodModeSelector->addChoice("USB");
    demodModeSelector->addChoice("DSB");
    demodModeSelector->addChoice("I/Q");
    demodModeSelector->setSelection("FM");
    demodModeSelector->setHelpTip("Choose modulation type: Frequency Modulation (Hotkey F), Amplitude Modulation (A) and Lower (L), Upper (U), Double Side-Band and more.");
    demodModeSelector->SetMinSize(wxSize(40,-1));
    demodModeSelector->SetMaxSize(wxSize(40,-1));
    demodTray->Add(demodModeSelector, 2, wxEXPAND | wxALL, 0);
    
#ifdef ENABLE_DIGITAL_LAB
    demodModeSelectorAdv = new ModeSelectorCanvas(demodPanel, attribList);
    demodModeSelectorAdv->addChoice("ASK");
    demodModeSelectorAdv->addChoice("APSK");
    demodModeSelectorAdv->addChoice("BPSK");
    demodModeSelectorAdv->addChoice("DPSK");
    demodModeSelectorAdv->addChoice("PSK");
    demodModeSelectorAdv->addChoice("FSK");
    demodModeSelectorAdv->addChoice("GMSK");
    demodModeSelectorAdv->addChoice("OOK");
    demodModeSelectorAdv->addChoice("ST");
    demodModeSelectorAdv->addChoice("SQAM");
    demodModeSelectorAdv->addChoice("QAM");
    demodModeSelectorAdv->addChoice("QPSK");
    demodModeSelectorAdv->setHelpTip("Choose advanced modulation types.");
    demodModeSelectorAdv->SetMinSize(wxSize(40,-1));
    demodModeSelectorAdv->SetMaxSize(wxSize(40,-1));
    demodTray->Add(demodModeSelectorAdv, 3, wxEXPAND | wxALL, 0);
#endif
            
    modemPropertiesUpdated.store(false);
    modemProps = new ModemProperties(demodPanel, wxID_ANY);
    modemProps->SetMinSize(wxSize(200,-1));
    modemProps->SetMaxSize(wxSize(200,-1));

    modemProps->Hide();
    demodTray->Add(modemProps, 15, wxEXPAND | wxALL, 0);
            
    wxGetApp().getDemodSpectrumProcessor()->setup(1024);
    demodSpectrumCanvas = new SpectrumCanvas(demodPanel, attribList);
    demodSpectrumCanvas->setView(wxGetApp().getConfig()->getCenterFreq(), 300000);
    demodVisuals->Add(demodSpectrumCanvas, 3, wxEXPAND | wxALL, 0);
    wxGetApp().getDemodSpectrumProcessor()->attachOutput(demodSpectrumCanvas->getVisualDataQueue());

    demodVisuals->AddSpacer(1);

    demodWaterfallCanvas = new WaterfallCanvas(demodPanel, attribList);
    demodWaterfallCanvas->setup(1024, 128);
    demodWaterfallCanvas->setView(wxGetApp().getConfig()->getCenterFreq(), 300000);
    demodWaterfallCanvas->attachSpectrumCanvas(demodSpectrumCanvas);
    demodWaterfallCanvas->setMinBandwidth(8000);
    demodSpectrumCanvas->attachWaterfallCanvas(demodWaterfallCanvas);
    demodVisuals->Add(demodWaterfallCanvas, 6, wxEXPAND | wxALL, 0);
    wxGetApp().getDemodSpectrumProcessor()->attachOutput(demodWaterfallCanvas->getVisualDataQueue());
    demodWaterfallCanvas->getVisualDataQueue()->set_max_num_items(3);

    demodVisuals->SetMinSize(wxSize(128,-1));

    demodTray->Add(demodVisuals, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(1);

    demodSignalMeter = new MeterCanvas(demodPanel, attribList);
    demodSignalMeter->setMax(DEMOD_SIGNAL_MAX);
    demodSignalMeter->setMin(DEMOD_SIGNAL_MIN);
    demodSignalMeter->setLevel(DEMOD_SIGNAL_MIN);
    demodSignalMeter->setInputValue(DEMOD_SIGNAL_MIN);
    demodSignalMeter->setHelpTip("Current Signal Level.  Click / Drag to set Squelch level.  Right-Click to Auto-Zero Squelch");
    demodSignalMeter->SetMinSize(wxSize(12,24));
    demodTray->Add(demodSignalMeter, 1, wxEXPAND | wxALL, 0);


    demodTray->AddSpacer(1);

    scopeCanvas = new ScopeCanvas(demodPanel, attribList);
    scopeCanvas->setHelpTip("Audio Visuals, drag left/right to toggle Scope or Spectrum.");
    scopeCanvas->SetMinSize(wxSize(128,-1));
    demodScopeTray->Add(scopeCanvas, 8, wxEXPAND | wxALL, 0);
    wxGetApp().getScopeProcessor()->setup(1024);
    wxGetApp().getScopeProcessor()->attachOutput(scopeCanvas->getInputQueue());

    demodScopeTray->AddSpacer(1);

    deltaLockButton = new ModeSelectorCanvas(demodPanel, attribList);
    deltaLockButton->addChoice(1, "V");
    deltaLockButton->setPadding(-1,-1);
    deltaLockButton->setHighlightColor(RGBA4f(0.8,0.8,0.2));
    deltaLockButton->setHelpTip("Delta Lock Toggle (V) - Enable to lock modem relative to center frequency.");
    deltaLockButton->setToggleMode(true);
    deltaLockButton->setSelection(-1);
    deltaLockButton->SetMinSize(wxSize(20,28));
    
    demodTunerTray->Add(deltaLockButton, 0, wxEXPAND | wxALL, 0);
    demodTunerTray->AddSpacer(1);
            
    demodTuner = new TuningCanvas(demodPanel, attribList);
    demodTuner->SetMinClientSize(wxSize(200,28));
    demodTunerTray->Add(demodTuner, 1, wxEXPAND | wxALL, 0);
            
    demodScopeTray->Add(demodTunerTray, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodScopeTray, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(1);

    wxBoxSizer *demodGainTray = new wxBoxSizer(wxVERTICAL);
            
    demodGainMeter = new MeterCanvas(demodPanel, attribList);
    demodGainMeter->setMax(2.0);
    demodGainMeter->setHelpTip("Current Demodulator Gain Level.  Click / Drag to set Gain level.");
    demodGainMeter->setShowUserInput(false);
    demodGainMeter->SetMinSize(wxSize(12,24));
    demodGainTray->Add(demodGainMeter, 8, wxEXPAND | wxALL, 0);

    demodGainTray->AddSpacer(1);
    
    soloModeButton = new ModeSelectorCanvas(demodPanel, attribList);
    soloModeButton->addChoice(1, "S");
    soloModeButton->setPadding(-1,-1);
    soloModeButton->setHighlightColor(RGBA4f(0.8,0.8,0.2));
    soloModeButton->setHelpTip("Solo Mode Toggle");
    soloModeButton->setToggleMode(true);
    soloModeButton->setSelection(-1);
    soloModeButton->SetMinSize(wxSize(12,28));
    
    demodGainTray->Add(soloModeButton, 1, wxEXPAND | wxALL, 0);

    demodGainTray->AddSpacer(1);

    demodMuteButton = new ModeSelectorCanvas(demodPanel, attribList);
    demodMuteButton->addChoice(1, "M");
    demodMuteButton->setPadding(-1,-1);
    demodMuteButton->setHighlightColor(RGBA4f(0.8,0.2,0.2));
    demodMuteButton->setHelpTip("Demodulator Mute Toggle");
    demodMuteButton->setToggleMode(true);
	demodMuteButton->setSelection(-1);
    demodMuteButton->SetMinSize(wxSize(12,28));
            
    demodGainTray->Add(demodMuteButton, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodGainTray, 1, wxEXPAND | wxALL, 0);
    
    demodPanel->SetSizer(demodTray);

//    vbox->Add(demodTray, 12, wxEXPAND | wxALL, 0);
//    vbox->AddSpacer(1);
            
    mainVisSplitter = new wxSplitterWindow( mainSplitter, wxID_VIS_SPLITTER, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE );
    mainVisSplitter->SetSashGravity(6.0/25.0);
    mainVisSplitter->SetMinimumPaneSize(1);
        
//    mainVisSplitter->Connect( wxEVT_IDLE, wxIdleEventHandler( AppFrame::mainVisSplitterIdle ), NULL, this );

    wxPanel *spectrumPanel = new wxPanel(mainVisSplitter, wxID_ANY);
    wxBoxSizer *spectrumSizer = new wxBoxSizer(wxHORIZONTAL);

    wxGetApp().getSpectrumProcessor()->setup(2048);
    spectrumCanvas = new SpectrumCanvas(spectrumPanel, attribList);
    spectrumCanvas->setShowDb(true);
    spectrumCanvas->setScaleFactorEnabled(true);
    wxGetApp().getSpectrumProcessor()->attachOutput(spectrumCanvas->getVisualDataQueue());
           
    wxBoxSizer *spectrumCtlTray = new wxBoxSizer(wxVERTICAL);
            
    peakHoldButton = new ModeSelectorCanvas(spectrumPanel, attribList);
    peakHoldButton->addChoice(1, "P");
    peakHoldButton->setPadding(-1,-1);
    peakHoldButton->setHighlightColor(RGBA4f(0.2,0.8,0.2));
    peakHoldButton->setHelpTip("Peak Hold Toggle");
    peakHoldButton->setToggleMode(true);
    peakHoldButton->setSelection(-1);
    peakHoldButton->SetMinSize(wxSize(12,24));

    spectrumCtlTray->Add(peakHoldButton, 1, wxEXPAND | wxALL, 0);
    spectrumCtlTray->AddSpacer(1);

    spectrumAvgMeter = new MeterCanvas(spectrumPanel, attribList);
    spectrumAvgMeter->setHelpTip("Spectrum averaging speed, click or drag to adjust.");
    spectrumAvgMeter->setMax(1.0);
    spectrumAvgMeter->setLevel(0.65);
    spectrumAvgMeter->setShowUserInput(false);
    spectrumAvgMeter->SetMinSize(wxSize(12,24));
            
    spectrumCtlTray->Add(spectrumAvgMeter, 8, wxEXPAND | wxALL, 0);

    spectrumSizer->Add(spectrumCanvas, 63, wxEXPAND | wxALL, 0);
    spectrumSizer->AddSpacer(1);
    spectrumSizer->Add(spectrumCtlTray, 1, wxEXPAND | wxALL, 0);
    spectrumPanel->SetSizer(spectrumSizer);
            
//    vbox->Add(spectrumSizer, 5, wxEXPAND | wxALL, 0);

//    vbox->AddSpacer(1);
            
    wxPanel *waterfallPanel = new wxPanel(mainVisSplitter, wxID_ANY);
    wxBoxSizer *wfSizer = new wxBoxSizer(wxHORIZONTAL);
           
    waterfallCanvas = new WaterfallCanvas(waterfallPanel, attribList);
    waterfallCanvas->setup(2048, 512);

    waterfallDataThread = new FFTVisualDataThread();

    waterfallDataThread->setInputQueue("IQDataInput", wxGetApp().getWaterfallVisualQueue());
    waterfallDataThread->setOutputQueue("FFTDataOutput", waterfallCanvas->getVisualDataQueue());
    waterfallDataThread->getProcessor()->setHideDC(true);

    t_FFTData = new std::thread(&FFTVisualDataThread::threadMain, waterfallDataThread);

    waterfallSpeedMeter = new MeterCanvas(waterfallPanel, attribList);
    waterfallSpeedMeter->setHelpTip("Waterfall speed, click or drag to adjust (max 1024 lines per second)");
    waterfallSpeedMeter->setMax(sqrt(1024));
    waterfallSpeedMeter->setLevel(sqrt(DEFAULT_WATERFALL_LPS));
    waterfallSpeedMeter->setShowUserInput(false);
    waterfallSpeedMeter->SetMinSize(wxSize(12,24));

    wfSizer->Add(waterfallCanvas, 63, wxEXPAND | wxALL, 0);
    wfSizer->AddSpacer(1);
    wfSizer->Add(waterfallSpeedMeter, 1, wxEXPAND | wxALL, 0);
    waterfallPanel->SetSizer(wfSizer);
            
//    vbox->Add(wfSizer, 20, wxEXPAND | wxALL, 0);

    mainVisSplitter->SplitHorizontally( spectrumPanel, waterfallPanel, 0 );
    mainSplitter->SplitHorizontally( demodPanel, mainVisSplitter );
            
    vbox->Add(mainSplitter, 1, wxEXPAND | wxALL, 0);
            
    // TODO: refactor these..
    waterfallCanvas->attachSpectrumCanvas(spectrumCanvas);
    spectrumCanvas->attachWaterfallCanvas(waterfallCanvas);

/*
    vbox->AddSpacer(1);
    testCanvas = new UITestCanvas(this, attribList);
    vbox->Add(testCanvas, 20, wxEXPAND | wxALL, 0);
// */
            
    this->SetSizer(vbox);

    //    SetIcon(wxICON(sample));

    // Make a menubar
    menuBar = new wxMenuBar;
    wxMenu *menu = new wxMenu;
    
    menu->Append(wxID_SDR_DEVICES, "SDR Devices");
    menu->AppendSeparator();
    menu->Append(wxID_OPEN, "&Open Session");
    menu->Append(wxID_SAVE, "&Save Session");
    menu->Append(wxID_SAVEAS, "Save Session &As..");
    menu->AppendSeparator();
    menu->Append(wxID_RESET, "&Reset Session");
            
#ifndef __APPLE__
    menu->AppendSeparator();
    menu->Append(wxID_CLOSE);
#endif
            
    menuBar->Append(menu, wxT("&File"));
            
    settingsMenu = new wxMenu;
          
    menuBar->Append(settingsMenu, wxT("&Settings"));
            
    menu = new wxMenu;

    std::vector<RtAudio::DeviceInfo>::iterator devices_i;
    std::map<int, RtAudio::DeviceInfo>::iterator mdevices_i;
    AudioThread::enumerateDevices(devices);

    int i = 0;

    for (devices_i = devices.begin(); devices_i != devices.end(); devices_i++) {
        if (devices_i->inputChannels) {
            inputDevices[i] = *devices_i;
        }
        if (devices_i->outputChannels) {
            outputDevices[i] = *devices_i;
        }
        i++;
    }

    for (mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
        wxMenuItem *itm = menu->AppendRadioItem(wxID_RT_AUDIO_DEVICE + mdevices_i->first, mdevices_i->second.name, wxT("Description?"));
        itm->SetId(wxID_RT_AUDIO_DEVICE + mdevices_i->first);
        if (mdevices_i->second.isDefaultOutput) {
            itm->Check(true);
        }
        outputDeviceMenuItems[mdevices_i->first] = itm;
    }

    menuBar->Append(menu, wxT("Audio &Output"));

    menu = new wxMenu;

    int themeId = wxGetApp().getConfig()->getTheme();
            
    menu->AppendRadioItem(wxID_THEME_DEFAULT, "Default")->Check(themeId==COLOR_THEME_DEFAULT);
    menu->AppendRadioItem(wxID_THEME_RADAR, "RADAR")->Check(themeId==COLOR_THEME_RADAR);
    menu->AppendRadioItem(wxID_THEME_BW, "Black & White")->Check(themeId==COLOR_THEME_BW);
    menu->AppendRadioItem(wxID_THEME_SHARP, "Sharp")->Check(themeId==COLOR_THEME_SHARP);
    menu->AppendRadioItem(wxID_THEME_RAD, "Rad")->Check(themeId==COLOR_THEME_RAD);
    menu->AppendRadioItem(wxID_THEME_TOUCH, "Touch")->Check(themeId==COLOR_THEME_TOUCH);
    menu->AppendRadioItem(wxID_THEME_HD, "HD")->Check(themeId==COLOR_THEME_HD);

    menuBar->Append(menu, wxT("&Color Scheme"));

    menu = new wxMenu;

#define NUM_RATES_DEFAULT 4
    unsigned int desired_rates[NUM_RATES_DEFAULT] = { 48000, 44100, 96000, 192000 };

    for (mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
        unsigned int desired_rate = 0;
        unsigned int desired_rank = NUM_RATES_DEFAULT + 1;

        for (std::vector<unsigned int>::iterator srate = mdevices_i->second.sampleRates.begin(); srate != mdevices_i->second.sampleRates.end();
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

    for (mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
        int menu_id = wxID_AUDIO_BANDWIDTH_BASE + wxID_AUDIO_DEVICE_MULTIPLIER * mdevices_i->first;
        wxMenu *subMenu = new wxMenu;
        menu->AppendSubMenu(subMenu, mdevices_i->second.name, wxT("Description?"));

        int j = 0;
        for (std::vector<unsigned int>::iterator srate = mdevices_i->second.sampleRates.begin(); srate != mdevices_i->second.sampleRates.end();
                srate++) {
            std::stringstream srateName;
            srateName << ((float) (*srate) / 1000.0f) << "kHz";
            wxMenuItem *itm = subMenu->AppendRadioItem(menu_id + j, srateName.str(), wxT("Description?"));

            if ((int)(*srate) == AudioThread::deviceSampleRate[mdevices_i->first]) {
                itm->Check(true);
            }
            audioSampleRateMenuItems[menu_id + j] = itm;

            j++;
        }
    }

    sampleRateMenu = new wxMenu;

    menuBar->Append(sampleRateMenu, wxT("Sample &Rate"));

    menuBar->Append(menu, wxT("Audio &Sample Rate"));

#ifdef USE_HAMLIB
            
    rigModel = wxGetApp().getConfig()->getRigModel();
    rigSerialRate = wxGetApp().getConfig()->getRigRate();
    rigPort = wxGetApp().getConfig()->getRigPort();
            
    rigMenu = new wxMenu;

    rigEnableMenuItem = rigMenu->AppendCheckItem(wxID_RIG_TOGGLE, wxT("Enable Rig"));

    rigMenu->Append(wxID_RIG_SDR_IF, wxT("SDR-IF"));

    rigControlMenuItem = rigMenu->AppendCheckItem(wxID_RIG_CONTROL, wxT("Control Rig"));
    rigControlMenuItem->Check(wxGetApp().getConfig()->getRigControlMode());
            
    rigFollowMenuItem = rigMenu->AppendCheckItem(wxID_RIG_FOLLOW, wxT("Follow Rig"));
    rigFollowMenuItem->Check(wxGetApp().getConfig()->getRigFollowMode());

    wxMenu *rigModelMenu = new wxMenu;
    RigList &rl = RigThread::enumerate();
    numRigs = rl.size();
            
    int modelMenuId = wxID_RIG_MODEL_BASE;
    for (RigList::const_iterator ri = rl.begin(); ri != rl.end(); ri++) {
        std::string modelString((*ri)->mfg_name);
        modelString.append(" ");
        modelString.append((*ri)->model_name);
        
        rigModelMenuItems[(*ri)->rig_model] = rigModelMenu->AppendRadioItem(modelMenuId, modelString, wxT("Description?"));
        
        if (rigModel == (*ri)->rig_model) {
            rigModelMenuItems[(*ri)->rig_model]->Check(true);
        }
        
        modelMenuId++;
    }

    rigMenu->AppendSubMenu(rigModelMenu, wxT("Model"));

    wxMenu *rigSerialMenu = new wxMenu;
            
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
    for (std::vector<int>::const_iterator rate_i = rigSerialRates.begin(); rate_i != rigSerialRates.end(); rate_i++) {
        std::string rateString;
        rateString.append(std::to_string((*rate_i)));
        rateString.append(" baud");
        
        rigSerialMenuItems[(*rate_i)] = rigSerialMenu->AppendRadioItem(rateMenuId, rateString, wxT("Description?"));
        
        if (rigSerialRate == (*rate_i)) {
            rigSerialMenuItems[(*rate_i)]->Check(true);
        }
        
        rateMenuId++;
    }
    
    rigMenu->AppendSubMenu(rigSerialMenu, wxT("Serial Rate"));

    rigPortMenuItem = rigMenu->Append(wxID_RIG_PORT, wxT("Control Port"));

    menuBar->Append(rigMenu, wxT("&Rig Control"));
#endif
            
    SetMenuBar(menuBar);

    CreateStatusBar();

    wxRect *win = wxGetApp().getConfig()->getWindow();
    if (win) {
        this->SetPosition(win->GetPosition());
        this->SetClientSize(win->GetSize());
    } else {
        SetClientSize(1280, 600);
        Centre();
    }
    bool max = wxGetApp().getConfig()->getWindowMaximized();

    if (max) {
        this->Maximize();
    }

    long long freqSnap = wxGetApp().getConfig()->getSnap();
    wxGetApp().setFrequencySnap(freqSnap);

    float spectrumAvg = wxGetApp().getConfig()->getSpectrumAvgSpeed();
            
    spectrumAvgMeter->setLevel(spectrumAvg);
    wxGetApp().getSpectrumProcessor()->setFFTAverageRate(spectrumAvg);
            
    int wflps =wxGetApp().getConfig()->getWaterfallLinesPerSec();
            
    waterfallSpeedMeter->setLevel(sqrt(wflps));
    waterfallDataThread->setLinesPerSecond(wflps);
    waterfallCanvas->setLinesPerSecond(wflps);
            
    ThemeMgr::mgr.setTheme(wxGetApp().getConfig()->getTheme());

    Show();

#ifdef _WIN32
    SetIcon(wxICON(frame_icon));
#endif

    wxAcceleratorEntry entries[3];
    entries[0].Set(wxACCEL_CTRL, (int) 'O', wxID_OPEN);
    entries[1].Set(wxACCEL_CTRL, (int) 'S', wxID_SAVE);
    entries[2].Set(wxACCEL_CTRL, (int) 'A', wxID_SAVEAS);

    wxAcceleratorTable accel(3, entries);
    SetAcceleratorTable(accel);
    deviceChanged.store(false);
    devInfo = NULL;
    wxGetApp().deviceSelector();
            
//    static const int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
//    wxLogStatus("Double-buffered display %s supported", wxGLCanvas::IsDisplaySupported(attribs) ? "is" : "not");
//    ShowFullScreen(true);
}

AppFrame::~AppFrame() {
    waterfallDataThread->terminate();
    t_FFTData->join();
}

void AppFrame::initDeviceParams(SDRDeviceInfo *devInfo) {
    this->devInfo = devInfo;
    deviceChanged.store(true);
}

void AppFrame::updateDeviceParams() {
    
    if (!deviceChanged.load()) {
        return;
    }
    
    int i = 0;
    SoapySDR::Device *soapyDev = devInfo->getSoapyDevice();
    
    // Build settings menu
    wxMenu *newSettingsMenu = new wxMenu;
    showTipMenuItem = newSettingsMenu->AppendCheckItem(wxID_SET_TIPS, "Show Hover Tips");
    showTipMenuItem->Check(wxGetApp().getConfig()->getShowTips());

    newSettingsMenu->Append(wxID_SET_FREQ_OFFSET, "Frequency Offset");

    if (devInfo->hasCORR(SOAPY_SDR_RX, 0)) {
        newSettingsMenu->Append(wxID_SET_PPM, "Device PPM");
    }

    if (devInfo->getDriver() != "rtlsdr") {
        iqSwapMenuItem = newSettingsMenu->AppendCheckItem(wxID_SET_IQSWAP, "I/Q Swap");
        iqSwapMenuItem->Check(wxGetApp().getSDRThread()->getIQSwap());
    }

    agcMenuItem = newSettingsMenu->AppendCheckItem(wxID_AGC_CONTROL, "Automatic Gain");
    agcMenuItem->Check(wxGetApp().getAGCMode());
    
    SoapySDR::ArgInfoList::const_iterator args_i;
    settingArgs = soapyDev->getSettingInfo();
    
    for (args_i = settingArgs.begin(); args_i != settingArgs.end(); args_i++) {
        SoapySDR::ArgInfo arg = (*args_i);
        std::string currentVal = soapyDev->readSetting(arg.key);
        if (arg.type == SoapySDR::ArgInfo::BOOL) {
            wxMenuItem *item = newSettingsMenu->AppendCheckItem(wxID_SETTINGS_BASE+i, arg.name, arg.description);
            item->Check(currentVal=="true");
            i++;
        } else if (arg.type == SoapySDR::ArgInfo::INT) {
            newSettingsMenu->Append(wxID_SETTINGS_BASE+i, arg.name, arg.description);
            i++;
        } else if (arg.type == SoapySDR::ArgInfo::FLOAT) {
            newSettingsMenu->Append(wxID_SETTINGS_BASE+i, arg.name, arg.description);
            i++;
        } else if (arg.type == SoapySDR::ArgInfo::STRING) {
            if (arg.options.size()) {
                wxMenu *subMenu = new wxMenu;
                int j = 0;
                for (std::vector<std::string>::iterator str_i = arg.options.begin(); str_i != arg.options.end(); str_i++) {
                    std::string optName = (*str_i);
                    std::string displayName = optName;
                    if (arg.optionNames.size()) {
                        displayName = arg.optionNames[j];
                    }
                    wxMenuItem *item = subMenu->AppendRadioItem(wxID_SETTINGS_BASE+i, displayName);
                    if (currentVal == (*str_i)) {
                        item->Check(true);
                    }
                    j++;
                    i++;
                }
                newSettingsMenu->AppendSubMenu(subMenu, arg.name, arg.description);
            } else {
                newSettingsMenu->Append(wxID_SETTINGS_BASE+i, arg.name, arg.description);
                i++;
            }
        }
    }
    settingsIdMax = wxID_SETTINGS_BASE+i;
    
    menuBar->Replace(1, newSettingsMenu, wxT("&Settings"));
    settingsMenu = newSettingsMenu;
    
    // Build sample rate menu
    sampleRates = devInfo->getSampleRates(SOAPY_SDR_RX, 0);
    sampleRateMenuItems.erase(sampleRateMenuItems.begin(),sampleRateMenuItems.end());
    
    wxMenu *newSampleRateMenu = new wxMenu;
    int ofs = 0;
    long sampleRate = wxGetApp().getSampleRate();
    bool checked = false;
    for (vector<long>::iterator i = sampleRates.begin(); i != sampleRates.end(); i++) {
        sampleRateMenuItems[wxID_BANDWIDTH_BASE+ofs] = newSampleRateMenu->AppendRadioItem(wxID_BANDWIDTH_BASE+ofs, frequencyToStr(*i));
        if (sampleRate == (*i)) {
            sampleRateMenuItems[wxID_BANDWIDTH_BASE+ofs]->Check(true);
            checked = true;
        }
        ofs++;
    }
    
    sampleRateMenuItems[wxID_BANDWIDTH_MANUAL] = newSampleRateMenu->AppendRadioItem(wxID_BANDWIDTH_MANUAL, wxT("Manual Entry")); 
    if (!checked) {
        sampleRateMenuItems[wxID_BANDWIDTH_MANUAL]->Check(true);
    }
   
    menuBar->Replace(4, newSampleRateMenu, wxT("Sample &Rate"));
    sampleRateMenu = newSampleRateMenu;

    if (!wxGetApp().getAGCMode()) {
        gainSpacerItem->Show(true);
        gainSizerItem->Show(true);
        gainSizerItem->SetMinSize(devInfo->getSoapyDevice()->listGains(SOAPY_SDR_RX,0).size()*50,0);
        demodTray->Layout();
        gainCanvas->updateGainUI();
        gainCanvas->Refresh();
        gainCanvas->Refresh();
    } else {
        gainSpacerItem->Show(false);
        gainSizerItem->Show(false);
        demodTray->Layout();
    }
    
    agcMenuItem->Check(wxGetApp().getAGCMode());
    

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
#endif

void AppFrame::OnMenu(wxCommandEvent& event) {
    if (event.GetId() >= wxID_RT_AUDIO_DEVICE && event.GetId() < wxID_RT_AUDIO_DEVICE + (int)devices.size()) {
        if (activeDemodulator) {
            activeDemodulator->setOutputDevice(event.GetId() - wxID_RT_AUDIO_DEVICE);
            activeDemodulator = NULL;
        }
    } else if (event.GetId() == wxID_SET_TIPS ) {
        if (wxGetApp().getConfig()->getShowTips()) {
            wxGetApp().getConfig()->setShowTips(false);
        } else {
            wxGetApp().getConfig()->setShowTips(true);
        }
    } else if (event.GetId() == wxID_SET_IQSWAP) {
        wxGetApp().getSDRThread()->setIQSwap(!wxGetApp().getSDRThread()->getIQSwap());
    } else if (event.GetId() == wxID_SET_FREQ_OFFSET) {
        long ofs = wxGetNumberFromUser("Shift the displayed frequency by this amount.\ni.e. -125000000 for -125 MHz", "Frequency (Hz)",
                "Frequency Offset", wxGetApp().getOffset(), -2000000000, 2000000000, this);
        if (ofs != -1) {
            wxGetApp().setOffset(ofs);
        }
    } else if (event.GetId() == wxID_AGC_CONTROL) {
        if (wxGetApp().getDevice() == NULL) {
            agcMenuItem->Check(true);
            return;
        }
        if (!wxGetApp().getAGCMode()) {
            wxGetApp().setAGCMode(true);
            gainSpacerItem->Show(false);
            gainSizerItem->Show(false);
            demodTray->Layout();
        } else {
            wxGetApp().setAGCMode(false);
            gainSpacerItem->Show(true);
            gainSizerItem->Show(true);
            gainSizerItem->SetMinSize(wxGetApp().getDevice()->getSoapyDevice()->listGains(SOAPY_SDR_RX, 0).size()*40,0);
            demodTray->Layout();
            gainCanvas->updateGainUI();
            gainCanvas->Refresh();
            gainCanvas->Refresh();
        }
    } else if (event.GetId() == wxID_SDR_DEVICES) {
        wxGetApp().deviceSelector();
    } else if (event.GetId() == wxID_SET_PPM) {
        long ofs = wxGetNumberFromUser("Frequency correction for device in PPM.\ni.e. -51 for -51 PPM\n\nNote: you can adjust PPM interactively\nby holding ALT over the frequency tuning bar.\n", "Parts per million (PPM)",
                "Frequency Correction", wxGetApp().getPPM(), -1000, 1000, this);
            wxGetApp().setPPM(ofs);
    } else if (event.GetId() == wxID_SAVE) {
        if (!currentSessionFile.empty()) {
            saveSession(currentSessionFile);
        } else {
            wxFileDialog saveFileDialog(this, _("Save XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (saveFileDialog.ShowModal() == wxID_CANCEL) {
                return;
            }
            saveSession(saveFileDialog.GetPath().ToStdString());
        }
    } else if (event.GetId() == wxID_OPEN) {
        wxFileDialog openFileDialog(this, _("Open XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() == wxID_CANCEL) {
            return;
        }
        loadSession(openFileDialog.GetPath().ToStdString());
    } else if (event.GetId() == wxID_SAVEAS) {
        wxFileDialog saveFileDialog(this, _("Save XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (saveFileDialog.ShowModal() == wxID_CANCEL) {
            return;
        }
        saveSession(saveFileDialog.GetPath().ToStdString());
    } else if (event.GetId() == wxID_RESET) {
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
        wxGetApp().getSpectrumProcessor()->setFFTAverageRate(0.65);
        spectrumAvgMeter->setLevel(0.65);
        demodModeSelector->Refresh();
        demodTuner->Refresh();
        SetTitle(CUBICSDR_TITLE);
        currentSessionFile = "";
    } else if (event.GetId() == wxID_CLOSE || event.GetId() == wxID_EXIT) {
        Close(false);
    } else if (event.GetId() == wxID_THEME_DEFAULT) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_DEFAULT);
    } else if (event.GetId() == wxID_THEME_SHARP) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_SHARP);
    } else if (event.GetId() == wxID_THEME_BW) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_BW);
    } else if (event.GetId() == wxID_THEME_RAD) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_RAD);
    } else if (event.GetId() == wxID_THEME_TOUCH) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_TOUCH);
    } else if (event.GetId() == wxID_THEME_HD) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_HD);
    } else if (event.GetId() == wxID_THEME_RADAR) {
        ThemeMgr::mgr.setTheme(COLOR_THEME_RADAR);
    }

    if (event.GetId() >= wxID_SETTINGS_BASE && event.GetId() < settingsIdMax) {
        int setIdx = event.GetId()-wxID_SETTINGS_BASE;
        int menuIdx = 0;
        for (std::vector<SoapySDR::ArgInfo>::iterator arg_i = settingArgs.begin(); arg_i != settingArgs.end(); arg_i++) {
            SoapySDR::ArgInfo &arg = (*arg_i);

            if (arg.type == SoapySDR::ArgInfo::STRING && arg.options.size() && setIdx >= menuIdx && setIdx < menuIdx+(int)arg.options.size()) {
                int optIdx = setIdx-menuIdx;
                wxGetApp().getSDRThread()->writeSetting(arg.key, arg.options[optIdx]);
                break;
            } else if (arg.type == SoapySDR::ArgInfo::STRING && arg.options.size()) {
                menuIdx += arg.options.size();
            } else if (menuIdx == setIdx) {
                if (arg.type == SoapySDR::ArgInfo::BOOL) {
                    wxGetApp().getSDRThread()->writeSetting(arg.key, (wxGetApp().getSDRThread()->readSetting(arg.key)=="true")?"false":"true");
                    break;
                } else if (arg.type == SoapySDR::ArgInfo::STRING) {
                    wxString stringVal = wxGetTextFromUser(arg.description, arg.name, wxGetApp().getSDRThread()->readSetting(arg.key));
                    if (stringVal.ToStdString() != "") {
                        wxGetApp().getSDRThread()->writeSetting(arg.key, stringVal.ToStdString());
                    }
                    break;
                } else if (arg.type == SoapySDR::ArgInfo::INT) {
                    int currentVal;
                    try {
                        currentVal = std::stoi(wxGetApp().getSDRThread()->readSetting(arg.key));
                    } catch (std::invalid_argument e) {
                        currentVal = 0;
                    }
                    int intVal = wxGetNumberFromUser(arg.description, arg.units, arg.name, currentVal, arg.range.minimum(), arg.range.maximum(), this);
                    if (intVal != -1) {
                        wxGetApp().getSDRThread()->writeSetting(arg.key, std::to_string(intVal));
                    }
                    break;
                } else if (arg.type == SoapySDR::ArgInfo::FLOAT) {
                    wxString floatVal = wxGetTextFromUser(arg.description, arg.name, wxGetApp().getSDRThread()->readSetting(arg.key));
                    try {
                        wxGetApp().getSDRThread()->writeSetting(arg.key, floatVal.ToStdString());
                    } catch (std::invalid_argument e) {
                        // ...
                    }
                    break;
                } else {
                    menuIdx++;
                }
            } else {
                menuIdx++;
            }
        }
    }
    
    if (event.GetId() >= wxID_THEME_DEFAULT && event.GetId() <= wxID_THEME_RADAR) {
    	demodTuner->Refresh();
    	demodModeSelector->Refresh();
        waterfallSpeedMeter->Refresh();
        spectrumAvgMeter->Refresh();
        gainCanvas->setThemeColors();
    }

    switch (event.GetId()) {
        case wxID_BANDWIDTH_MANUAL:
            int rateHigh, rateLow;
            
            SDRDeviceInfo *dev = wxGetApp().getDevice();
            if (dev == NULL) {
                break;
            }
            
            std::vector<long> sampleRates = dev->getSampleRates(SOAPY_SDR_RX, 0);
            
            rateLow = 2000000;
            rateHigh = 30000000;
            
            if (sampleRates.size()) {
                rateLow = sampleRates[0];
                rateHigh = sampleRates[sampleRates.size()-1];
            }

            long bw = wxGetNumberFromUser("\n" + dev->getName() + "\n\n  "
                                          + "min: " + std::to_string(rateLow) + " Hz"
                                          + ", max: " + std::to_string(rateHigh) + " Hz\n",
                                          "Sample Rate in Hz",
                                          "Manual Sample Rate Entry",
                                          wxGetApp().getSampleRate(),
                                          rateLow,
                                          rateHigh,
                                          this);
            if (bw != -1) {
                wxGetApp().setSampleRate(bw);
            }
            break;
    }
    
    if (event.GetId() >= wxID_BANDWIDTH_BASE && event.GetId() < wxID_BANDWIDTH_BASE + (int)sampleRates.size()) {
        wxGetApp().setSampleRate(sampleRates[event.GetId()-wxID_BANDWIDTH_BASE]);
    }
    
    if (event.GetId() >= wxID_AUDIO_BANDWIDTH_BASE) {
        int evId = event.GetId();
        std::vector<RtAudio::DeviceInfo>::iterator devices_i;
        std::map<int, RtAudio::DeviceInfo>::iterator mdevices_i;

        int i = 0;
        for (mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
            int menu_id = wxID_AUDIO_BANDWIDTH_BASE + wxID_AUDIO_DEVICE_MULTIPLIER * mdevices_i->first;

            int j = 0;
            for (std::vector<unsigned int>::iterator srate = mdevices_i->second.sampleRates.begin(); srate != mdevices_i->second.sampleRates.end();
                    srate++) {

                if (evId == menu_id + j) {
                    //audioSampleRateMenuItems[menu_id+j];
                    //std::cout << "Would set audio sample rate on device " << mdevices_i->second.name << " (" << mdevices_i->first << ") to " << (*srate) << "Hz" << std::endl;
                    AudioThread::setDeviceSampleRate(mdevices_i->first, *srate);
                }

                j++;
            }
            i++;
        }
    }
    
#ifdef USE_HAMLIB

    bool resetRig = false;
    if (event.GetId() >= wxID_RIG_MODEL_BASE && event.GetId() < wxID_RIG_MODEL_BASE+numRigs) {
        int rigIdx = event.GetId()-wxID_RIG_MODEL_BASE;
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
    }

    if (event.GetId() >= wxID_RIG_SERIAL_BASE && event.GetId() < wxID_RIG_SERIAL_BASE+rigSerialRates.size()) {
        int serialIdx = event.GetId()-wxID_RIG_SERIAL_BASE;
        rigSerialRate = rigSerialRates[serialIdx];
        resetRig = true;
    }

    if (event.GetId() == wxID_RIG_PORT) {
        wxString stringVal = wxGetTextFromUser("Rig Serial / COM / Address", "Rig Control Port", rigPort);
        std::string rigPortStr = stringVal.ToStdString();
        if (rigPortStr != "") {
            rigPort = rigPortStr;
            resetRig = true;
        }
    }

    if (event.GetId() == wxID_RIG_TOGGLE) {
        resetRig = false;
        if (!wxGetApp().rigIsActive()) {
            enableRig();
        } else {
            disableRig();
        }
    }
    
    if (event.GetId() == wxID_RIG_SDR_IF) {
        if (devInfo != nullptr) {
            std::string deviceId = devInfo->getDeviceId();
            DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(deviceId);
            long long freqRigIF = wxGetNumberFromUser("Rig SDR-IF Frequency", "Frequency (Hz)", "Frequency", devConfig->getRigIF(rigModel), 0, 2000000000);
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
    }
    
    if (wxGetApp().rigIsActive() && resetRig) {
        wxGetApp().stopRig();
        wxGetApp().initRig(rigModel, rigPort, rigSerialRate);
    }
#endif

}

void AppFrame::OnClose(wxCloseEvent& event) {
    wxGetApp().closeDeviceSelector();

    wxGetApp().getDemodSpectrumProcessor()->removeOutput(demodSpectrumCanvas->getVisualDataQueue());
    wxGetApp().getDemodSpectrumProcessor()->removeOutput(demodWaterfallCanvas->getVisualDataQueue());
    wxGetApp().getSpectrumProcessor()->removeOutput(spectrumCanvas->getVisualDataQueue());

    wxGetApp().getConfig()->setWindow(this->GetPosition(), this->GetClientSize());
    wxGetApp().getConfig()->setWindowMaximized(this->IsMaximized());
    wxGetApp().getConfig()->setTheme(ThemeMgr::mgr.getTheme());
    wxGetApp().getConfig()->setSnap(wxGetApp().getFrequencySnap());
    wxGetApp().getConfig()->setCenterFreq(wxGetApp().getFrequency());
    wxGetApp().getConfig()->setSpectrumAvgSpeed(wxGetApp().getSpectrumProcessor()->getFFTAverageRate());
    wxGetApp().getConfig()->setWaterfallLinesPerSec(waterfallDataThread->getLinesPerSecond());
    wxGetApp().getConfig()->setManualDevices(SDREnumerator::getManuals());
#ifdef USE_HAMLIB
    wxGetApp().getConfig()->setRigEnabled(rigEnableMenuItem->IsChecked());
    wxGetApp().getConfig()->setRigModel(rigModel);
    wxGetApp().getConfig()->setRigRate(rigSerialRate);
    wxGetApp().getConfig()->setRigPort(rigPort);
    wxGetApp().getConfig()->setRigFollowMode(rigFollowMenuItem->IsChecked());
    wxGetApp().getConfig()->setRigControlMode(rigControlMenuItem->IsChecked());
#endif
    wxGetApp().getConfig()->save();
    event.Skip();
}

void AppFrame::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
    new AppFrame();
}

void AppFrame::OnThread(wxCommandEvent& event) {
    event.Skip();
}

void AppFrame::OnIdle(wxIdleEvent& event) {

    if (deviceChanged.load()) {
        updateDeviceParams();
    }
    
    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    if (demod && demod->isModemInitialized()) {
        if (demod->isTracking()) {
            if (spectrumCanvas->getViewState()) {
                long long diff = abs(demod->getFrequency() - spectrumCanvas->getCenterFrequency()) + (demod->getBandwidth()/2) + (demod->getBandwidth()/4);

                if (diff > spectrumCanvas->getBandwidth()/2) {
                    if (demod->getBandwidth() > (int)spectrumCanvas->getBandwidth()) {
                        diff = abs(demod->getFrequency() - spectrumCanvas->getCenterFrequency());
                    } else {
                        diff = diff - spectrumCanvas->getBandwidth()/2;
                    }
                    spectrumCanvas->moveCenterFrequency((demod->getFrequency() < spectrumCanvas->getCenterFrequency())?diff:-diff);
                    demod->setTracking(false);
                }
            } else {
                demod->setTracking(false);
            }
        }
        
        if (demod->getBandwidth() != wxGetApp().getDemodMgr().getLastBandwidth()) {
            wxGetApp().getDemodMgr().setLastBandwidth(demod->getBandwidth());
        }

        if (demod != activeDemodulator) {
            demodSignalMeter->setInputValue(demod->getSquelchLevel());
            demodGainMeter->setInputValue(demod->getGain());
            int outputDevice = demod->getOutputDevice();
            scopeCanvas->setDeviceName(outputDevices[outputDevice].name);
            outputDeviceMenuItems[outputDevice]->Check(true);
            std::string dType = demod->getDemodulatorType();
            demodModeSelector->setSelection(dType);
#ifdef ENABLE_DIGITAL_LAB
            demodModeSelectorAdv->setSelection(dType);
#endif
            deltaLockButton->setSelection(demod->isDeltaLock()?1:-1);
            demodMuteButton->setSelection(demod->isMuted()?1:-1);
            modemPropertiesUpdated.store(true);
            demodTuner->setHalfBand(dType=="USB" || dType=="LSB");
        }
        if (demodWaterfallCanvas->getDragState() == WaterfallCanvas::WF_DRAG_NONE) {
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

            if (centerFreq != demodWaterfallCanvas->getCenterFrequency()) {
                demodWaterfallCanvas->setCenterFrequency(centerFreq);
                demodSpectrumCanvas->setCenterFrequency(centerFreq);
            }
            std::string dSelection = demodModeSelector->getSelectionLabel();
#ifdef ENABLE_DIGITAL_LAB
            std::string dSelectionadv = demodModeSelectorAdv->getSelectionLabel();

            // basic demodulators
            if (dSelection != "" && dSelection != demod->getDemodulatorType()) {
                demod->setDemodulatorType(dSelection);
                demodTuner->setHalfBand(dSelection=="USB" || dSelection=="LSB");
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
                    demod->setDeltaLockOfs(demod->getFrequency()-wxGetApp().getFrequency());
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
                if (wxGetApp().getSoloMode() != (soloMode==1)) {
                    soloModeButton->setSelection(wxGetApp().getSoloMode()?1:-1);
                    soloModeButton->Refresh();
                }
            }
            
            demodWaterfallCanvas->setBandwidth(demodBw);
            demodSpectrumCanvas->setBandwidth(demodBw);
        }
        demodSignalMeter->setLevel(demod->getSignalLevel());
        demodGainMeter->setLevel(demod->getGain());
        if (demodSignalMeter->inputChanged()) {
            demod->setSquelchLevel(demodSignalMeter->getInputValue());
        }
        if (demodGainMeter->inputChanged()) {
            demod->setGain(demodGainMeter->getInputValue());
            demodGainMeter->setLevel(demodGainMeter->getInputValue());
        }
        activeDemodulator = demod;
    } else if (demod) {
        // Wait state for current demodulator modem to activate..
    } else {
        DemodulatorMgr *mgr = &wxGetApp().getDemodMgr();

        std::string dSelection = demodModeSelector->getSelectionLabel();
#ifdef ENABLE_DIGITAL_LAB
        std::string dSelectionadv = demodModeSelectorAdv->getSelectionLabel();

        // basic demodulators
        if (dSelection != "" && dSelection != mgr->getLastDemodulatorType()) {
            mgr->setLastDemodulatorType(dSelection);
            mgr->setLastBandwidth(Modem::getModemDefaultSampleRate(dSelection));
            demodTuner->setHalfBand(dSelection=="USB" || dSelection=="LSB");
            demodModeSelectorAdv->setSelection(-1);
        }
        // advanced demodulators
        else if(dSelectionadv != "" && dSelectionadv != mgr->getLastDemodulatorType()) {
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
        demodGainMeter->setLevel(mgr->getLastGain());
        if (demodSignalMeter->inputChanged()) {
            mgr->setLastSquelchLevel(demodSignalMeter->getInputValue());
        }
        if (demodGainMeter->inputChanged()) {
            mgr->setLastGain(demodGainMeter->getInputValue());
            demodGainMeter->setLevel(demodGainMeter->getInputValue());
        }

        if (wxGetApp().getFrequency() != demodWaterfallCanvas->getCenterFrequency()) {
            demodWaterfallCanvas->setCenterFrequency(wxGetApp().getFrequency());
            demodSpectrumCanvas->setCenterFrequency(wxGetApp().getFrequency());
        }
        if (spectrumCanvas->getViewState() && abs(wxGetApp().getFrequency()-spectrumCanvas->getCenterFrequency()) > (wxGetApp().getSampleRate()/2)) {
            spectrumCanvas->setCenterFrequency(wxGetApp().getFrequency());
            waterfallCanvas->setCenterFrequency(wxGetApp().getFrequency());
        }
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

    scopeCanvas->setPPMMode(demodTuner->isAltDown());
    
    scopeCanvas->setShowDb(spectrumCanvas->getShowDb());
    wxGetApp().getScopeProcessor()->setScopeEnabled(scopeCanvas->scopeVisible());
    wxGetApp().getScopeProcessor()->setSpectrumEnabled(scopeCanvas->spectrumVisible());
    wxGetApp().getAudioVisualQueue()->set_max_num_items((scopeCanvas->scopeVisible()?1:0) + (scopeCanvas->spectrumVisible()?1:0));
    
    wxGetApp().getScopeProcessor()->run();

    SpectrumVisualProcessor *proc = wxGetApp().getSpectrumProcessor();

    if (spectrumAvgMeter->inputChanged()) {
        float val = spectrumAvgMeter->getInputValue();
        if (val < 0.01) {
            val = 0.01;
        }
        if (val > 0.99) {
            val = 0.99;
        }
        spectrumAvgMeter->setLevel(val);
        proc->setFFTAverageRate(val);

        GetStatusBar()->SetStatusText(wxString::Format(wxT("Spectrum averaging speed changed to %0.2f%%."),val*100.0));
    }
    
    SpectrumVisualProcessor *dproc = wxGetApp().getDemodSpectrumProcessor();
    
    dproc->setView(demodWaterfallCanvas->getViewState(), demodWaterfallCanvas->getCenterFrequency(),demodWaterfallCanvas->getBandwidth());

    SpectrumVisualProcessor *wproc = waterfallDataThread->getProcessor();
    
    if (waterfallSpeedMeter->inputChanged()) {
        float val = waterfallSpeedMeter->getInputValue();
        waterfallSpeedMeter->setLevel(val);
        waterfallDataThread->setLinesPerSecond((int)ceil(val*val));
        waterfallCanvas->setLinesPerSecond((int)ceil(val*val));
        GetStatusBar()->SetStatusText(wxString::Format(wxT("Waterfall max speed changed to %d lines per second."),(int)ceil(val*val)));
    }

    wproc->setView(waterfallCanvas->getViewState(), waterfallCanvas->getCenterFrequency(), waterfallCanvas->getBandwidth());
    wxGetApp().getSDRPostThread()->setIQVisualRange(waterfallCanvas->getCenterFrequency(), waterfallCanvas->getBandwidth());
    
    proc->setView(wproc->isView(), wproc->getCenterFrequency(), wproc->getBandwidth());
    
    demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
    
    if (modemPropertiesUpdated.load() && demod && demod->isModemInitialized()) {
        modemProps->initProperties(demod->getModemArgs());
        modemPropertiesUpdated.store(false);
        demodTray->Layout();
#if ENABLE_DIGITAL_LAB
        if (demod->getModemType() == "digital") {
            ModemDigitalOutputConsole *outp = (ModemDigitalOutputConsole *)demod->getOutput();
            if (!outp->getDialog()) {
                outp->setTitle(demod->getDemodulatorType() + ": " + frequencyToStr(demod->getFrequency()));
                outp->setDialog(new DigitalConsole(this, outp));
            }
            demod->showOutput();
        }
#endif
    }
    
    int peakHoldMode = peakHoldButton->getSelection();
    if (peakHoldButton->modeChanged()) {
        wxGetApp().getSpectrumProcessor()->setPeakHold(peakHoldMode == 1);
        peakHoldButton->clearModeChanged();
    }
    
#if USE_HAMLIB
    if (rigEnableMenuItem->IsChecked()) {
        if (!wxGetApp().rigIsActive()) {
            rigEnableMenuItem->Check(false);
            wxGetApp().getConfig()->setRigEnabled(false);
        }
    }
#endif
    
    if (!this->IsActive()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    event.RequestMore();
}


void AppFrame::OnDoubleClickSash(wxSplitterEvent& event)
{
    wxWindow *a, *b;
    wxSplitterWindow *w = NULL;
    float g = 0.5;

    if (event.GetId() == wxID_MAIN_SPLITTER) {
        w = mainSplitter;
        g = 10.0/37.0;
    } else if (event.GetId() == wxID_VIS_SPLITTER) {
        w = mainVisSplitter;
        g = 6.0/25.0;
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

void AppFrame::saveSession(std::string fileName) {
    DataTree s("cubicsdr_session");
    DataNode *header = s.rootNode()->newChild("header");
    *header->newChild("version") = std::string(CUBICSDR_VERSION);
    *header->newChild("center_freq") = wxGetApp().getFrequency();
    *header->newChild("sample_rate") = wxGetApp().getSampleRate();
    
    DataNode *demods = s.rootNode()->newChild("demodulators");

    std::vector<DemodulatorInstance *> &instances = wxGetApp().getDemodMgr().getDemodulators();
    std::vector<DemodulatorInstance *>::iterator instance_i;
    for (instance_i = instances.begin(); instance_i != instances.end(); instance_i++) {
        DataNode *demod = demods->newChild("demodulator");
        *demod->newChild("bandwidth") = (*instance_i)->getBandwidth();
        *demod->newChild("frequency") = (*instance_i)->getFrequency();
        *demod->newChild("type") = (*instance_i)->getDemodulatorType();
        *demod->newChild("squelch_level") = (*instance_i)->getSquelchLevel();
        *demod->newChild("squelch_enabled") = (*instance_i)->isSquelchEnabled() ? 1 : 0;
        *demod->newChild("output_device") = outputDevices[(*instance_i)->getOutputDevice()].name;
        *demod->newChild("gain") = (*instance_i)->getGain();
        *demod->newChild("muted") = (*instance_i)->isMuted() ? 1 : 0;
        if ((*instance_i)->isDeltaLock()) {
            *demod->newChild("delta_lock") = (*instance_i)->isDeltaLock() ? 1 : 0;
            *demod->newChild("delta_ofs") = (*instance_i)->getDeltaLockOfs();
        }
        if ((*instance_i) == wxGetApp().getDemodMgr().getLastActiveDemodulator()) {
            *demod->newChild("active") = 1;
        }

        ModemSettings saveSettings = (*instance_i)->readModemSettings();
        if (saveSettings.size()) {
            DataNode *settingsNode = demod->newChild("settings");
            for (ModemSettings::const_iterator msi = saveSettings.begin(); msi != saveSettings.end(); msi++) {
                *settingsNode->newChild(msi->first.c_str()) = msi->second;
            }
        }
    }

    s.SaveToFileXML(fileName);

    currentSessionFile = fileName;
    std::string filePart = fileName.substr(fileName.find_last_of(filePathSeparator) + 1);
    GetStatusBar()->SetStatusText(wxString::Format(wxT("Saved session: %s"), currentSessionFile.c_str()));
    SetTitle(wxString::Format(wxT("%s: %s"), CUBICSDR_TITLE, filePart.c_str()));
}

bool AppFrame::loadSession(std::string fileName) {
    DataTree l;
    if (!l.LoadFromFileXML(fileName)) {
        return false;
    }

    wxGetApp().getDemodMgr().terminateAll();

    try {
        DataNode *header = l.rootNode()->getNext("header");

        std::string version(*header->getNext("version"));
        std::cout << "Loading " << version << " session file" << std::endl;

        long long center_freq = *header->getNext("center_freq");
        std::cout << "\tCenter Frequency: " << center_freq << std::endl;
        
        if (header->hasAnother("sample_rate")) {
            int sample_rate = *header->getNext("sample_rate");
            
            SDRDeviceInfo *dev = wxGetApp().getSDRThread()->getDevice();
            if (dev) {
                // Try for a reasonable default sample rate.
                sample_rate = dev->getSampleRateNear(SOAPY_SDR_RX, 0, sample_rate);
                wxGetApp().setSampleRate(sample_rate);
                deviceChanged.store(true);
            } else {
                wxGetApp().setSampleRate(sample_rate);
            }
            
        }

        wxGetApp().setFrequency(center_freq);

        DataNode *demodulators = l.rootNode()->getNext("demodulators");

        int numDemodulators = 0;
        DemodulatorInstance *loadedDemod = NULL;
        DemodulatorInstance *newDemod = NULL;
        
        while (demodulators->hasAnother("demodulator")) {
            DataNode *demod = demodulators->getNext("demodulator");

            if (!demod->hasAnother("bandwidth") || !demod->hasAnother("frequency")) {
                continue;
            }

            long bandwidth = *demod->getNext("bandwidth");
            long long freq = *demod->getNext("frequency");
            float squelch_level = demod->hasAnother("squelch_level") ? (float) *demod->getNext("squelch_level") : 0;
            int squelch_enabled = demod->hasAnother("squelch_enabled") ? (int) *demod->getNext("squelch_enabled") : 0;
            int muted = demod->hasAnother("muted") ? (int) *demod->getNext("muted") : 0;
            int delta_locked = demod->hasAnother("delta_lock") ? (int) *demod->getNext("delta_lock") : 0;
            int delta_ofs = demod->hasAnother("delta_ofs") ? (int) *demod->getNext("delta_ofs") : 0;
            std::string output_device = demod->hasAnother("output_device") ? string(*(demod->getNext("output_device"))) : "";
            float gain = demod->hasAnother("gain") ? (float) *demod->getNext("gain") : 1.0;
            
            std::string type = "FM";

            DataNode *demodTypeNode = demod->hasAnother("type")?demod->getNext("type"):nullptr;
            
            if (demodTypeNode->element()->getDataType() == DATA_INT) {
                int legacyType = *demodTypeNode;
                int legacyStereo = demod->hasAnother("stereo") ? (int) *demod->getNext("stereo") : 0;
                switch (legacyType) {   // legacy demod ID
                    case 1: type = legacyStereo?"FMS":"FM"; break;
                    case 2: type = "AM"; break;
                    case 3: type = "LSB"; break;
                    case 4: type = "USB"; break;
                    case 5: type = "DSB"; break;
                    case 6: type = "ASK"; break;
                    case 7: type = "APSK"; break;
                    case 8: type = "BPSK"; break;
                    case 9: type = "DPSK"; break;
                    case 10: type = "PSK"; break;
                    case 11: type = "OOK"; break;
                    case 12: type = "ST"; break;
                    case 13: type = "SQAM"; break;
                    case 14: type = "QAM"; break;
                    case 15: type = "QPSK"; break;
                    case 16: type = "I/Q"; break;
                    default: type = "FM"; break;
                }
            } else if (demodTypeNode->element()->getDataType() == DATA_STRING) {
                demodTypeNode->element()->get(type);
            }

            ModemSettings mSettings;
            
            if (demod->hasAnother("settings")) {
                DataNode *modemSettings = demod->getNext("settings");
                for (int msi = 0, numSettings = modemSettings->numChildren(); msi < numSettings; msi++) {
                    DataNode *settingNode = modemSettings->child(msi);
                    std::string keyName = settingNode->getName();
                    std::string strSettingValue = settingNode->element()->toString();
                    
                    if (keyName != "" && strSettingValue != "") {
                        mSettings[keyName] = strSettingValue;
                    }
                }
            }
            
            newDemod = wxGetApp().getDemodMgr().newThread();

            if (demod->hasAnother("active")) {
                loadedDemod = newDemod;
            }

            numDemodulators++;
            newDemod->setDemodulatorType(type);
            newDemod->writeModemSettings(mSettings);
            newDemod->setBandwidth(bandwidth);
            newDemod->setFrequency(freq);
            newDemod->setGain(gain);
            newDemod->updateLabel(freq);
            newDemod->setMuted(muted?true:false);
            if (delta_locked) {
                newDemod->setDeltaLock(true);
                newDemod->setDeltaLockOfs(delta_ofs);
            }
            if (squelch_enabled) {
                newDemod->setSquelchEnabled(true);
                newDemod->setSquelchLevel(squelch_level);
            }
            
            bool found_device = false;
            std::map<int, RtAudio::DeviceInfo>::iterator i;
            for (i = outputDevices.begin(); i != outputDevices.end(); i++) {
                if (i->second.name == output_device) {
                    newDemod->setOutputDevice(i->first);
                    found_device = true;
                }
            }

            if (!found_device) {
                std::cout << "\tWarning: named output device '" << output_device << "' was not found. Using default output.";
            }

            newDemod->run();
            newDemod->setActive(false);
            wxGetApp().bindDemodulator(newDemod);

            std::cout << "\tAdded demodulator at frequency " << freq << " type " << type << std::endl;
            std::cout << "\t\tBandwidth: " << bandwidth << std::endl;
            std::cout << "\t\tSquelch Level: " << squelch_level << std::endl;
            std::cout << "\t\tSquelch Enabled: " << (squelch_enabled ? "true" : "false") << std::endl;
            std::cout << "\t\tOutput Device: " << output_device << std::endl;
        }
        
        DemodulatorInstance *focusDemod = loadedDemod?loadedDemod:newDemod;
        
        if (focusDemod) {
            focusDemod->setActive(true);
            focusDemod->setFollow(true);
            focusDemod->setTracking(true);
            wxGetApp().getDemodMgr().setActiveDemodulator(focusDemod, false);
        }
    } catch (DataInvalidChildException &e) {
        std::cout << e.what() << std::endl;
        return false;
    } catch (DataTypeMismatchException &e) {
        std::cout << e.what() << std::endl;
        return false;
    }

    currentSessionFile = fileName;

    std::string filePart = fileName.substr(fileName.find_last_of(filePathSeparator) + 1);

    GetStatusBar()->SetStatusText(wxString::Format(wxT("Loaded session file: %s"), currentSessionFile.c_str()));
    SetTitle(wxString::Format(wxT("%s: %s"), CUBICSDR_TITLE, filePart.c_str()));

    return true;
}

FFTVisualDataThread *AppFrame::getWaterfallDataThread() {
    return waterfallDataThread;
}

void AppFrame::updateModemProperties(ModemArgInfoList args) {
    newModemArgs = args;
    modemPropertiesUpdated.store(true);
}

void AppFrame::setMainWaterfallFFTSize(int fftSize) {
    wxGetApp().getSpectrumProcessor()->setFFTSize(fftSize);
    spectrumCanvas->setFFTSize(fftSize);
    waterfallDataThread->getProcessor()->setFFTSize(fftSize);
    waterfallCanvas->setFFTSize(fftSize);
}


void AppFrame::refreshGainUI() {
    gainCanvas->updateGainUI();
    gainCanvas->Refresh();
}


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

int AppFrame::OnGlobalKeyDown(wxKeyEvent &event) {
    if (!this->IsActive()) {
        return -1;
    }
    
    DemodulatorInstance *demod = nullptr, *lastDemod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
    int snap = wxGetApp().getFrequencySnap();
    
    if (event.ShiftDown()) {
        if (snap != 1) {
            snap /= 2;
        }
    }
    
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
            if (lastDemod) {
                lastDemod->setFrequency(lastDemod->getFrequency()+snap);
            }
            return 1;
            break;
        case '[':
            if (lastDemod) {
                lastDemod->setFrequency(lastDemod->getFrequency()-snap);
            }
            return 1;
            break;
        case 'A':
        case 'F':
        case 'L':
        case 'U':
        case 'S':
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
            lastDemod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
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
            break;
        default:
            break;
    }
    
    if (demodTuner->getMouseTracker()->mouseInView()) {
        demodTuner->OnKeyDown(event);
    } else if (waterfallCanvas->getMouseTracker()->mouseInView()) {
        waterfallCanvas->OnKeyDown(event);
    }
    
    return 1;
}

int AppFrame::OnGlobalKeyUp(wxKeyEvent &event) {
    if (!this->IsActive()) {
        return -1;
    }

    DemodulatorInstance *lastDemod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
    
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
            if (wxGetApp().getDemodMgr().getActiveDemodulator()) {
                lastDemod = wxGetApp().getDemodMgr().getActiveDemodulator();
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
            } else {
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
        case ']':
        case '[':
            return 1;
        default:
            break;
    }
    
    if (demodTuner->getMouseTracker()->mouseInView()) {
        demodTuner->OnKeyUp(event);
    } else if (waterfallCanvas->getMouseTracker()->mouseInView()) {
        waterfallCanvas->OnKeyUp(event);
    }
    
    
    // TODO: Catch key-ups outside of original target

    return 1;
}


void AppFrame::setWaterfallLinesPerSecond(int lps) {
    waterfallSpeedMeter->setUserInputValue(sqrt(lps));
}

void AppFrame::setSpectrumAvgSpeed(double avg) {
    spectrumAvgMeter->setUserInputValue(avg);
}

