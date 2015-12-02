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


AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, CUBICSDR_TITLE), activeDemodulator(NULL) {

#ifdef __linux__
    SetIcon(wxICON(cubicsdr));
#endif

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodVisuals = new wxBoxSizer(wxVERTICAL);
    demodTray = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *demodScopeTray = new wxBoxSizer(wxVERTICAL);

    int attribList[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };

    mainSplitter = new wxSplitterWindow( this, wxID_MAIN_SPLITTER, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE );
    mainSplitter->SetSashGravity(12.0/37.0);
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
    demodModeSelector->setHelpTip("Choose modulation type: Frequency Modulation, Amplitude Modulation and Lower, Upper or Double Side-Band.");
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
    demodSignalMeter->setHelpTip("Current Signal Level.  Click / Drag to set Squelch level.");
    demodSignalMeter->SetMinSize(wxSize(12,24));
    demodTray->Add(demodSignalMeter, 1, wxEXPAND | wxALL, 0);


    demodTray->AddSpacer(1);

    scopeCanvas = new ScopeCanvas(demodPanel, attribList);
    scopeCanvas->setHelpTip("Audio Visuals, drag left/right to toggle Scope or Spectrum.");
    scopeCanvas->SetMinSize(wxSize(128,-1));
    demodScopeTray->Add(scopeCanvas, 8, wxEXPAND | wxALL, 0);
    wxGetApp().getScopeProcessor()->setup(2048);
    wxGetApp().getScopeProcessor()->attachOutput(scopeCanvas->getInputQueue());

    demodScopeTray->AddSpacer(1);

    demodTuner = new TuningCanvas(demodPanel, attribList);
    demodTuner->setHelpTip("Testing tuner");
    demodTuner->SetMinClientSize(wxSize(200,24));
    demodScopeTray->Add(demodTuner, 1, wxEXPAND | wxALL, 0);

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

    demodMuteButton = new ModeSelectorCanvas(demodPanel, attribList);
    demodMuteButton->addChoice(1, "M");
    demodMuteButton->setPadding(-1,-1);
    demodMuteButton->setHighlightColor(RGBA4f(0.8,0.2,0.2));
    demodMuteButton->setHelpTip("Demodulator Mute Toggle");
    demodMuteButton->setToggleMode(true);
	demodMuteButton->setSelection(-1);
    demodMuteButton->SetMinSize(wxSize(12,24));
            
    demodGainTray->Add(demodMuteButton, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodGainTray, 1, wxEXPAND | wxALL, 0);
    
    demodPanel->SetSizer(demodTray);

//    vbox->Add(demodTray, 12, wxEXPAND | wxALL, 0);
//    vbox->AddSpacer(1);
            
    mainVisSplitter = new wxSplitterWindow( mainSplitter, wxID_VIS_SPLITTER, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE );
    mainVisSplitter->SetSashGravity(5.0/25.0);
    mainVisSplitter->SetMinimumPaneSize(1);
        
//    mainVisSplitter->Connect( wxEVT_IDLE, wxIdleEventHandler( AppFrame::mainVisSplitterIdle ), NULL, this );

    wxPanel *spectrumPanel = new wxPanel(mainVisSplitter, wxID_ANY);
    wxBoxSizer *spectrumSizer = new wxBoxSizer(wxHORIZONTAL);

    wxGetApp().getSpectrumProcessor()->setup(2048);
    spectrumCanvas = new SpectrumCanvas(spectrumPanel, attribList);
    spectrumCanvas->setShowDb(true);
    spectrumCanvas->setScaleFactorEnabled(true);
    wxGetApp().getSpectrumProcessor()->attachOutput(spectrumCanvas->getVisualDataQueue());
            
    spectrumAvgMeter = new MeterCanvas(spectrumPanel, attribList);
    spectrumAvgMeter->setHelpTip("Spectrum averaging speed, click or drag to adjust.");
    spectrumAvgMeter->setMax(1.0);
    spectrumAvgMeter->setLevel(0.65);
    spectrumAvgMeter->setShowUserInput(false);
    spectrumAvgMeter->SetMinSize(wxSize(12,24));


    spectrumSizer->Add(spectrumCanvas, 63, wxEXPAND | wxALL, 0);
    spectrumSizer->AddSpacer(1);
    spectrumSizer->Add(spectrumAvgMeter, 1, wxEXPAND | wxALL, 0);
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

    //    waterfallCanvas->SetFocusFromKbd();
    waterfallCanvas->SetFocus();

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
    int desired_rates[NUM_RATES_DEFAULT] = { 48000, 44100, 96000, 192000 };

    for (mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
        int desired_rate = 0;
        int desired_rank = NUM_RATES_DEFAULT + 1;

        for (std::vector<unsigned int>::iterator srate = mdevices_i->second.sampleRates.begin(); srate != mdevices_i->second.sampleRates.end();
                srate++) {
            for (i = 0; i < NUM_RATES_DEFAULT; i++) {
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

            if ((*srate) == AudioThread::deviceSampleRate[mdevices_i->first]) {
                itm->Check(true);
            }
            audioSampleRateMenuItems[menu_id + j] = itm;

            j++;
        }
    }

    sampleRateMenu = new wxMenu;

    menuBar->Append(sampleRateMenu, wxT("&Input Bandwidth"));

    menuBar->Append(menu, wxT("Audio &Bandwidth"));

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
    
    
    // Build settings menu
    wxMenu *newSettingsMenu = new wxMenu;
    newSettingsMenu->Append(wxID_SET_FREQ_OFFSET, "Frequency Offset");
    if (devInfo->getRxChannel()->hasCORR()) {
        newSettingsMenu->Append(wxID_SET_PPM, "Device PPM");
    }
    
    agcMenuItem = newSettingsMenu->AppendCheckItem(wxID_AGC_CONTROL, "Automatic Gain");
    agcMenuItem->Check(wxGetApp().getAGCMode());
    
    SoapySDR::ArgInfoList::const_iterator args_i;
    
    int i = 0;
    settingArgs = devInfo->getSettingsArgInfo();
    for (args_i = settingArgs.begin(); args_i != settingArgs.end(); args_i++) {
        SoapySDR::ArgInfo arg = (*args_i);
        std::string currentVal = wxGetApp().getSDRThread()->readSetting(arg.key);
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
                        item->Check();
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
    sampleRates = devInfo->getRxChannel()->getSampleRates();
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
   
    menuBar->Replace(4, newSampleRateMenu, wxT("&Input Bandwidth"));
    sampleRateMenu = newSampleRateMenu;

    if (!wxGetApp().getAGCMode()) {
        gainSpacerItem->Show(true);
        gainSizerItem->Show(true);
        gainSizerItem->SetMinSize(devInfo->getRxChannel()->getGains().size()*50,0);
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
    
    deviceChanged.store(false);
}


void AppFrame::OnMenu(wxCommandEvent& event) {
    if (event.GetId() >= wxID_RT_AUDIO_DEVICE && event.GetId() < wxID_RT_AUDIO_DEVICE + devices.size()) {
        if (activeDemodulator) {
            activeDemodulator->setOutputDevice(event.GetId() - wxID_RT_AUDIO_DEVICE);
            activeDemodulator = NULL;
        }
    } else if (event.GetId() == wxID_SET_FREQ_OFFSET) {
        long ofs = wxGetNumberFromUser("Shift the displayed frequency by this amount.\ni.e. -125000000 for -125 MHz", "Frequency (Hz)",
                "Frequency Offset", wxGetApp().getOffset(), -2000000000, 2000000000, this);
        if (ofs != -1) {
            wxGetApp().setOffset(ofs);
            wxGetApp().saveConfig();
        }
    } else if (event.GetId() == wxID_SET_DS_OFF) {
//        wxGetApp().setDirectSampling(0);
//        wxGetApp().saveConfig();
    } else if (event.GetId() == wxID_SET_DS_I) {
//        wxGetApp().setDirectSampling(1);
//        wxGetApp().saveConfig();
    } else if (event.GetId() == wxID_SET_DS_Q) {
//        wxGetApp().setDirectSampling(2);
//        wxGetApp().saveConfig();
    } else if (event.GetId() == wxID_SET_SWAP_IQ) {
//        bool swap_state = !wxGetApp().getSwapIQ();
//        wxGetApp().setSwapIQ(swap_state);
//        wxGetApp().saveConfig();
//        iqSwapMenuItem->Check(swap_state);
    } else if (event.GetId() == wxID_AGC_CONTROL) {
        if (wxGetApp().getDevice() == NULL) {
            agcMenuItem->Check();
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
            gainSizerItem->SetMinSize(wxGetApp().getDevice()->getRxChannel()->getGains().size()*40,0);
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
            wxGetApp().saveConfig();
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

            if (arg.type == SoapySDR::ArgInfo::STRING && arg.options.size() && setIdx >= menuIdx && setIdx < menuIdx+arg.options.size()) {
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
                    menuIdx++;
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
            
            SDRDeviceChannel *chan = dev->getRxChannel();
            
            rateLow = 2000000;
            rateHigh = 30000000;
            
            if (chan->getSampleRates().size()) {
                rateLow = chan->getSampleRates()[0];
                rateHigh = chan->getSampleRates()[chan->getSampleRates().size()-1];
            }

            long bw = wxGetNumberFromUser("\n" + dev->getName() + "\n\n  "
                                          + "min: " + std::to_string(rateLow) + " Hz"
                                          + ", max: " + std::to_string(rateHigh) + " Hz\n",
                                          "Sample Rate in Hz",
                                          "Manual Bandwidth Entry",
                                          wxGetApp().getSampleRate(),
                                          rateLow,
                                          rateHigh,
                                          this);
            if (bw != -1) {
                wxGetApp().setSampleRate(bw);
            }
            break;
    }
    
    if (event.GetId() >= wxID_BANDWIDTH_BASE && event.GetId() < wxID_BANDWIDTH_BASE+sampleRates.size()) {
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
                    if (demod->getBandwidth() > spectrumCanvas->getBandwidth()) {
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
            demodMuteButton->setSelection(demod->isMuted()?1:-1);
            modemPropertiesUpdated.store(true);
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
                demodModeSelectorAdv->setSelection(-1);
            }
            // advanced demodulators
			else if (dSelectionadv != "" && dSelectionadv != demod->getDemodulatorType()) {
				demod->setDemodulatorType(dSelectionadv);
				demodModeSelector->setSelection(-1);
            }
#else
            // basic demodulators
            if (dSelection != "" && dSelection != demod->getDemodulatorType()) {
                demod->setDemodulatorType(dSelection);
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
                } else if (!demod->isMuted() && muteMode == 1) {
                    demodMuteButton->setSelection(-1);
                    wxGetApp().getDemodMgr().setLastMuted(demod->isMuted());
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
            demodModeSelectorAdv->setSelection(-1);
        }
        // advanced demodulators
        else if(dSelectionadv != "" && dSelectionadv != mgr->getLastDemodulatorType()) {
            mgr->setLastDemodulatorType(dSelectionadv);
            demodModeSelector->setSelection(-1);
        }
#else
        // basic demodulators
        if (dSelection != "" && dSelection != mgr->getLastDemodulatorType()) {
            mgr->setLastDemodulatorType(dSelection);
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

    if (demodTuner->getMouseTracker()->mouseInView()) {
        if (!demodTuner->HasFocus()) {
            demodTuner->SetFocus();
        }
    } else if (!wxGetApp().isDeviceSelectorOpen() && (!modemProps || !modemProps->isMouseInView())) {
		if (!waterfallCanvas->HasFocus()) {
			waterfallCanvas->SetFocus();
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
    
    proc->setView(waterfallCanvas->getViewState());
    proc->setBandwidth(waterfallCanvas->getBandwidth());
    proc->setCenterFrequency(waterfallCanvas->getCenterFrequency());
    
    SpectrumVisualProcessor *dproc = wxGetApp().getDemodSpectrumProcessor();
    
    dproc->setView(demodWaterfallCanvas->getViewState());
    dproc->setBandwidth(demodWaterfallCanvas->getBandwidth());
    dproc->setCenterFrequency(demodWaterfallCanvas->getCenterFrequency());

    SpectrumVisualProcessor *wproc = waterfallDataThread->getProcessor();
    
    if (waterfallSpeedMeter->inputChanged()) {
        float val = waterfallSpeedMeter->getInputValue();
        waterfallSpeedMeter->setLevel(val);
        waterfallDataThread->setLinesPerSecond((int)ceil(val*val));
        waterfallCanvas->setLinesPerSecond((int)ceil(val*val));
        GetStatusBar()->SetStatusText(wxString::Format(wxT("Waterfall max speed changed to %d lines per second."),(int)ceil(val*val)));
    }

    wproc->setView(waterfallCanvas->getViewState());
    wproc->setBandwidth(waterfallCanvas->getBandwidth());
    wproc->setCenterFrequency(waterfallCanvas->getCenterFrequency());
    wxGetApp().getSDRPostThread()->setIQVisualRange(waterfallCanvas->getCenterFrequency(), waterfallCanvas->getBandwidth());
    
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
    
    if (!this->IsActive()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
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
        g = 12.0/37.0;
    } else if (event.GetId() == wxID_VIS_SPLITTER) {
        w = mainVisSplitter;
        g = 7.4/37.0;
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
        long long center_freq = *header->getNext("center_freq");

        std::cout << "Loading " << version << " session file" << std::endl;
        std::cout << "\tCenter Frequency: " << center_freq << std::endl;

        wxGetApp().setFrequency(center_freq);

        DataNode *demodulators = l.rootNode()->getNext("demodulators");

        int numDemodulators = 0;
        DemodulatorInstance *loadedDemod = NULL;
        
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
                    std::string strSettingValue = "";
                    
                    int dataType = settingNode->element()->getDataType();
                    
                    try {
                        if (dataType == DATA_STRING) {
                            settingNode->element()->get(strSettingValue);
                        } else if (dataType == DATA_INT || dataType == DATA_LONG || dataType == DATA_LONGLONG) {
                            long long intSettingValue = *settingNode;
                            strSettingValue = std::to_string(intSettingValue);
                        } else if (dataType == DATA_FLOAT || dataType == DATA_DOUBLE) {
                            double floatSettingValue = *settingNode;
                            strSettingValue = std::to_string(floatSettingValue);
                        } else {
                            std::cout << "Unhandled setting data type: " << dataType  << std::endl;
                        }
                    } catch (DataTypeMismatchException e) {
                        std::cout << "Setting data type mismatch: " << dataType  << std::endl;
                    }
                    
                    if (keyName != "" && strSettingValue != "") {
                        mSettings[keyName] = strSettingValue;
                    }
                }
            }
            
            DemodulatorInstance *newDemod = wxGetApp().getDemodMgr().newThread();
            loadedDemod = newDemod;
            numDemodulators++;
            newDemod->writeModemSettings(mSettings);
            newDemod->setDemodulatorType(type);
            newDemod->setBandwidth(bandwidth);
            newDemod->setFrequency(freq);
            newDemod->setGain(gain);
            newDemod->updateLabel(freq);
            newDemod->setMuted(muted?true:false);
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
        
        if ((numDemodulators == 1) && loadedDemod) {
            loadedDemod->setActive(true);
            loadedDemod->setFollow(true);
            loadedDemod->setTracking(true);
            wxGetApp().getDemodMgr().setActiveDemodulator(loadedDemod);
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

