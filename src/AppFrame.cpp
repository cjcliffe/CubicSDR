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
#include "SDRThread.h"
#include "DemodulatorMgr.h"
#include "AudioThread.h"
#include "CubicSDR.h"
#include "DataTree.h"
#include "ColorTheme.h"

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
wxEND_EVENT_TABLE()

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, CUBICSDR_TITLE), activeDemodulator(NULL) {

#ifdef __linux__
    SetIcon(wxICON(cubicsdr));
#endif

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodVisuals = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodTray = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *demodScopeTray = new wxBoxSizer(wxVERTICAL);

    int attribList[] = { WX_GL_RGBA, WX_GL_STENCIL_SIZE, 8, WX_GL_BUFFER_SIZE, 24, WX_GL_DOUBLEBUFFER, 0 };

    demodModeSelector = new ModeSelectorCanvas(this, attribList);
    demodModeSelector->addChoice(DEMOD_TYPE_FM, "FM");
    demodModeSelector->addChoice(DEMOD_TYPE_AM, "AM");
    demodModeSelector->addChoice(DEMOD_TYPE_LSB, "LSB");
    demodModeSelector->addChoice(DEMOD_TYPE_USB, "USB");
    demodModeSelector->addChoice(DEMOD_TYPE_DSB, "DSB");
    demodModeSelector->addChoice(DEMOD_TYPE_RAW, "I/Q");
    demodModeSelector->setSelection(DEMOD_TYPE_FM);
    demodModeSelector->setHelpTip("Choose modulation type: Frequency Modulation, Amplitude Modulation and Lower, Upper or Double Side-Band.");
    demodTray->Add(demodModeSelector, 2, wxEXPAND | wxALL, 0);

    wxGetApp().getDemodSpectrumProcessor()->setup(1024);
    demodSpectrumCanvas = new SpectrumCanvas(this, attribList);
    demodSpectrumCanvas->setView(wxGetApp().getConfig()->getCenterFreq(), 300000);
    demodVisuals->Add(demodSpectrumCanvas, 3, wxEXPAND | wxALL, 0);
    wxGetApp().getDemodSpectrumProcessor()->attachOutput(demodSpectrumCanvas->getVisualDataQueue());

    demodVisuals->AddSpacer(1);

    demodWaterfallCanvas = new WaterfallCanvas(this, attribList);
    demodWaterfallCanvas->setup(1024, 128);
    demodWaterfallCanvas->setView(wxGetApp().getConfig()->getCenterFreq(), 300000);
    demodWaterfallCanvas->attachSpectrumCanvas(demodSpectrumCanvas);
    demodSpectrumCanvas->attachWaterfallCanvas(demodWaterfallCanvas);
    demodVisuals->Add(demodWaterfallCanvas, 6, wxEXPAND | wxALL, 0);
    wxGetApp().getDemodSpectrumProcessor()->attachOutput(demodWaterfallCanvas->getVisualDataQueue());

    demodTray->Add(demodVisuals, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(1);

    demodSignalMeter = new MeterCanvas(this, attribList);
    demodSignalMeter->setMax(0.5);
    demodSignalMeter->setHelpTip("Current Signal Level.  Click / Drag to set Squelch level.");
    demodTray->Add(demodSignalMeter, 1, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(1);

    scopeCanvas = new ScopeCanvas(this, attribList);
    demodScopeTray->Add(scopeCanvas, 8, wxEXPAND | wxALL, 0);
    wxGetApp().getScopeProcessor()->attachOutput(scopeCanvas->getInputQueue());

    demodScopeTray->AddSpacer(1);

    demodTuner = new TuningCanvas(this, attribList);
    demodTuner->setHelpTip("Testing tuner");
    demodScopeTray->Add(demodTuner, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodScopeTray, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(1);

    wxBoxSizer *demodGainTray = new wxBoxSizer(wxVERTICAL);
            
    demodGainMeter = new MeterCanvas(this, attribList);
    demodGainMeter->setMax(2.0);
    demodGainMeter->setHelpTip("Current Demodulator Gain Level.  Click / Drag to set Gain level.");
    demodGainMeter->setShowUserInput(false);
    demodGainTray->Add(demodGainMeter, 8, wxEXPAND | wxALL, 0);

    demodGainTray->AddSpacer(1);

    demodMuteButton = new ModeSelectorCanvas(this, attribList);
    demodMuteButton->addChoice(1, "M");
    demodMuteButton->setHelpTip("Demodulator Mute Toggle");
    demodMuteButton->setToggleMode(true);
          
    demodGainTray->Add(demodMuteButton, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodGainTray, 1, wxEXPAND | wxALL, 0);
            
    vbox->Add(demodTray, 12, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(1);

    wxBoxSizer *spectrumSizer = new wxBoxSizer(wxHORIZONTAL);
    wxGetApp().getSpectrumProcessor()->setup(2048);
    spectrumCanvas = new SpectrumCanvas(this, attribList);
    spectrumCanvas->setShowDb(true);
    wxGetApp().getSpectrumProcessor()->attachOutput(spectrumCanvas->getVisualDataQueue());
            
    spectrumAvgMeter = new MeterCanvas(this, attribList);
    spectrumAvgMeter->setHelpTip("Spectrum averaging speed, click or drag to adjust.");
    spectrumAvgMeter->setMax(1.0);
    spectrumAvgMeter->setLevel(0.65);
    spectrumAvgMeter->setShowUserInput(false);

    spectrumSizer->Add(spectrumCanvas, 63, wxEXPAND | wxALL, 0);
    spectrumSizer->AddSpacer(1);
    spectrumSizer->Add(spectrumAvgMeter, 1, wxEXPAND | wxALL, 0);
            
    vbox->Add(spectrumSizer, 5, wxEXPAND | wxALL, 0);

    vbox->AddSpacer(1);
            
    wxBoxSizer *wfSizer = new wxBoxSizer(wxHORIZONTAL);

    waterfallCanvas = new WaterfallCanvas(this, attribList);
    waterfallCanvas->setup(2048, 512);

    waterfallDataThread = new FFTVisualDataThread();
    t_FFTData = new std::thread(&FFTVisualDataThread::threadMain, waterfallDataThread);

    waterfallDataThread->setInputQueue("IQDataInput", wxGetApp().getWaterfallVisualQueue());
    waterfallDataThread->setOutputQueue("FFTDataOutput", waterfallCanvas->getVisualDataQueue());
            
    waterfallSpeedMeter = new MeterCanvas(this, attribList);
    waterfallSpeedMeter->setHelpTip("Waterfall speed, click or drag to adjust (max 1024 lines per second)");
    waterfallSpeedMeter->setMax(sqrt(1024));
    waterfallSpeedMeter->setLevel(sqrt(DEFAULT_WATERFALL_LPS));
    waterfallSpeedMeter->setShowUserInput(false);

    wfSizer->Add(waterfallCanvas, 63, wxEXPAND | wxALL, 0);
    wfSizer->AddSpacer(1);
    wfSizer->Add(waterfallSpeedMeter, 1, wxEXPAND | wxALL, 0);
            
    vbox->Add(wfSizer, 20, wxEXPAND | wxALL, 0);

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
    wxMenuBar *menuBar = new wxMenuBar;
    wxMenu *menu = new wxMenu;
    
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
            
    menu = new wxMenu;
    
    menu->Append(wxID_SET_FREQ_OFFSET, "Frequency Offset");
    menu->Append(wxID_SET_PPM, "Device PPM");
    iqSwapMenuItem = menu->AppendCheckItem(wxID_SET_SWAP_IQ, "Swap I/Q");
            
    wxMenu *dsMenu = new wxMenu;
    
    directSamplingMenuItems[0] = dsMenu->AppendRadioItem(wxID_SET_DS_OFF, "Off");
    directSamplingMenuItems[1] = dsMenu->AppendRadioItem(wxID_SET_DS_I, "I-ADC");
    directSamplingMenuItems[2] = dsMenu->AppendRadioItem(wxID_SET_DS_Q, "Q-ADC");
    
    menu->AppendSubMenu(dsMenu, "Direct Sampling");

    menuBar->Append(menu, wxT("&Settings"));
            
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
            
    sampleRateMenuItems[wxID_BANDWIDTH_250K] = menu->AppendRadioItem(wxID_BANDWIDTH_250K, "250k");
    sampleRateMenuItems[wxID_BANDWIDTH_1000M] = menu->AppendRadioItem(wxID_BANDWIDTH_1000M, "1.0M");
    sampleRateMenuItems[wxID_BANDWIDTH_1500M] = menu->AppendRadioItem(wxID_BANDWIDTH_1024M, "1.024M");
    sampleRateMenuItems[wxID_BANDWIDTH_1024M] = menu->AppendRadioItem(wxID_BANDWIDTH_1500M, "1.5M");
    sampleRateMenuItems[wxID_BANDWIDTH_1800M] = menu->AppendRadioItem(wxID_BANDWIDTH_1800M, "1.8M");
    sampleRateMenuItems[wxID_BANDWIDTH_1920M] = menu->AppendRadioItem(wxID_BANDWIDTH_1920M, "1.92M");
    sampleRateMenuItems[wxID_BANDWIDTH_2000M] = menu->AppendRadioItem(wxID_BANDWIDTH_2000M, "2.0M");
    sampleRateMenuItems[wxID_BANDWIDTH_2048M] = menu->AppendRadioItem(wxID_BANDWIDTH_2048M, "2.048M");
    sampleRateMenuItems[wxID_BANDWIDTH_2160M] = menu->AppendRadioItem(wxID_BANDWIDTH_2160M, "2.16M");
    sampleRateMenuItems[wxID_BANDWIDTH_2400M] = menu->AppendRadioItem(wxID_BANDWIDTH_2400M, "2.4M");
    sampleRateMenuItems[wxID_BANDWIDTH_2560M] = menu->AppendRadioItem(wxID_BANDWIDTH_2560M, "2.56M");
    sampleRateMenuItems[wxID_BANDWIDTH_2880M] = menu->AppendRadioItem(wxID_BANDWIDTH_2880M, "2.88M");
//    sampleRateMenuItems[wxID_BANDWIDTH_3000M] = menu->AppendRadioItem(wxID_BANDWIDTH_3000M, "3.0M");
    sampleRateMenuItems[wxID_BANDWIDTH_3200M] = menu->AppendRadioItem(wxID_BANDWIDTH_3200M, "3.2M");

    sampleRateMenuItems[wxID_BANDWIDTH_2400M]->Check(true);

    menuBar->Append(menu, wxT("&Input Bandwidth"));

    std::vector<SDRDeviceInfo *> *devs = wxGetApp().getDevices();
    std::vector<SDRDeviceInfo *>::iterator devs_i;

    if (devs->size() > 1) {

        menu = new wxMenu;

        int p = 0;
        for (devs_i = devs->begin(); devs_i != devs->end(); devs_i++) {
            std::string devName = (*devs_i)->getName();
            if ((*devs_i)->isAvailable()) {
                devName.append(": ");
                devName.append((*devs_i)->getProduct());
                devName.append(" [");
                devName.append((*devs_i)->getSerial());
                devName.append("]");
            } else {
                devName.append(" (In Use?)");
            }

            menu->AppendRadioItem(wxID_DEVICE_ID + p, devName)->Check(wxGetApp().getDevice() == p);
            p++;
        }

        menuBar->Append(menu, wxT("Input &Device"));
    }

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
        new wxMenu;
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

//    static const int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
//    wxLogStatus("Double-buffered display %s supported", wxGLCanvas::IsDisplaySupported(attribs) ? "is" : "not");
//    ShowFullScreen(true);
}

AppFrame::~AppFrame() {
    waterfallDataThread->terminate();
    t_FFTData->join();
}


void AppFrame::initDeviceParams(std::string deviceId) {
    DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(deviceId);
    
    int dsMode = devConfig->getDirectSampling();
    
    if (dsMode > 0 && dsMode <= 2) {
        directSamplingMenuItems[devConfig->getDirectSampling()]->Check();
    }
    
    if (devConfig->getIQSwap()) {
        iqSwapMenuItem->Check();
    }
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
        wxGetApp().setDirectSampling(0);
        wxGetApp().saveConfig();
    } else if (event.GetId() == wxID_SET_DS_I) {
        wxGetApp().setDirectSampling(1);
        wxGetApp().saveConfig();
    } else if (event.GetId() == wxID_SET_DS_Q) {
        wxGetApp().setDirectSampling(2);
        wxGetApp().saveConfig();
    } else if (event.GetId() == wxID_SET_SWAP_IQ) {
        bool swap_state = !wxGetApp().getSwapIQ();
        wxGetApp().setSwapIQ(swap_state);
        wxGetApp().saveConfig();
        iqSwapMenuItem->Check(swap_state);
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
        wxGetApp().getDemodMgr().setLastDemodulatorType(DEMOD_TYPE_FM);
        demodModeSelector->setSelection(1);
        wxGetApp().getDemodMgr().setLastStereo(false);
        wxGetApp().getDemodMgr().setLastBandwidth(DEFAULT_DEMOD_BW);
        wxGetApp().getDemodMgr().setLastGain(1.0);
        wxGetApp().getDemodMgr().setLastSquelchLevel(0);
        waterfallCanvas->setBandwidth(wxGetApp().getSampleRate());
        waterfallCanvas->setCenterFrequency(wxGetApp().getFrequency());
        spectrumCanvas->setBandwidth(wxGetApp().getSampleRate());
        spectrumCanvas->setCenterFrequency(wxGetApp().getFrequency());
        waterfallDataThread->setLinesPerSecond(DEFAULT_WATERFALL_LPS);
        waterfallSpeedMeter->setLevel(sqrt(DEFAULT_WATERFALL_LPS));
        wxGetApp().getSpectrumProcessor()->setFFTAverageRate(0.65);
        spectrumAvgMeter->setLevel(0.65);
        demodModeSelector->Refresh();
        demodTuner->Refresh();
        SetTitle(CUBICSDR_TITLE);
        currentSessionFile = "";
    } else if (event.GetId() == wxID_EXIT) {
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

    if (event.GetId() >= wxID_THEME_DEFAULT && event.GetId() <= wxID_THEME_RADAR) {
    	demodTuner->Refresh();
    	demodModeSelector->Refresh();
    }

    switch (event.GetId()) {
        case wxID_BANDWIDTH_250K:
            wxGetApp().setSampleRate(250000);
            break;
        case wxID_BANDWIDTH_1000M:
            wxGetApp().setSampleRate(1000000);
            break;
        case wxID_BANDWIDTH_1024M:
            wxGetApp().setSampleRate(1024000);
            break;
        case wxID_BANDWIDTH_1500M:
            wxGetApp().setSampleRate(1500000);
            break;
        case wxID_BANDWIDTH_1800M:
            wxGetApp().setSampleRate(1800000);
            break;
        case wxID_BANDWIDTH_1920M:
            wxGetApp().setSampleRate(1920000);
            break;
        case wxID_BANDWIDTH_2000M:
            wxGetApp().setSampleRate(2000000);
            break;
        case wxID_BANDWIDTH_2048M:
            wxGetApp().setSampleRate(2048000);
            break;
        case wxID_BANDWIDTH_2160M:
            wxGetApp().setSampleRate(2160000);
            break;
        case wxID_BANDWIDTH_2400M:
            wxGetApp().setSampleRate(2400000);
            break;
        case wxID_BANDWIDTH_2560M:
            wxGetApp().setSampleRate(2560000);
            break;
        case wxID_BANDWIDTH_2880M:
            wxGetApp().setSampleRate(2880000);
            break;
//        case wxID_BANDWIDTH_3000M:
//            wxGetApp().setSampleRate(3000000);
//            break;
        case wxID_BANDWIDTH_3200M:
            wxGetApp().setSampleRate(3200000);
            break;
    }

    std::vector<SDRDeviceInfo *> *devs = wxGetApp().getDevices();
    if (event.GetId() >= wxID_DEVICE_ID && event.GetId() <= wxID_DEVICE_ID + devs->size()) {
        int devId = event.GetId() - wxID_DEVICE_ID;
        wxGetApp().setDevice(devId);

        SDRDeviceInfo *dev = (*wxGetApp().getDevices())[devId];
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
        
        int dsMode = devConfig->getDirectSampling();
        
        if (dsMode >= 0 && dsMode <= 2) {
            directSamplingMenuItems[devConfig->getDirectSampling()]->Check();
        }
        
        iqSwapMenuItem->Check(devConfig->getIQSwap());
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

    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    if (demod) {
        DemodulatorInstance *demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

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

        if (demod != activeDemodulator) {
            demodSignalMeter->setInputValue(demod->getSquelchLevel());
            demodGainMeter->setInputValue(demod->getGain());
            int outputDevice = demod->getOutputDevice();
            scopeCanvas->setDeviceName(outputDevices[outputDevice].name);
            outputDeviceMenuItems[outputDevice]->Check(true);
            int dType = demod->getDemodulatorType();
            demodModeSelector->setSelection(dType);
            demodMuteButton->setSelection(demod->isMuted()?1:-1);
        }
        if (demodWaterfallCanvas->getDragState() == WaterfallCanvas::WF_DRAG_NONE) {
            long long centerFreq = demod->getFrequency();
            unsigned int demodBw = (unsigned int) ceil((float) demod->getBandwidth() * 2.25);

            if (demod->getDemodulatorType() == DEMOD_TYPE_USB) {
                demodBw /= 2;
                centerFreq += demod->getBandwidth() / 4;
            }

            if (demod->getDemodulatorType() == DEMOD_TYPE_LSB) {
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
            int dSelection = demodModeSelector->getSelection();
            if (dSelection != -1 && dSelection != demod->getDemodulatorType()) {
                demod->setDemodulatorType(dSelection);
            }

            int muteMode = demodMuteButton->getSelection();
            if (demodMuteButton->modeChanged()) {
                if (demod->isMuted() && muteMode == -1) {
                    demod->setMuted(false);
                } else if (!demod->isMuted() && muteMode == 1) {
                    demod->setMuted(true);
                }
                demodMuteButton->clearModeChanged();
            } else {
                if (demod->isMuted() && muteMode == -1) {
                    demodMuteButton->setSelection(1);
                } else if (!demod->isMuted() && muteMode == 1) {
                    demodMuteButton->setSelection(-1);
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
    } else {
        DemodulatorMgr *mgr = &wxGetApp().getDemodMgr();

        int dSelection = demodModeSelector->getSelection();
        if (dSelection != -1 && dSelection != mgr->getLastDemodulatorType()) {
            mgr->setLastDemodulatorType(dSelection);
        }
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
    }

    if (demodTuner->getMouseTracker()->mouseInView()) {
        if (!demodTuner->HasFocus()) {
            demodTuner->SetFocus();
        }
    } else if (!waterfallCanvas->HasFocus()) {
        waterfallCanvas->SetFocus();
    }

    scopeCanvas->setPPMMode(demodTuner->isAltDown());
    
    wxGetApp().getScopeProcessor()->run();
    wxGetApp().getSpectrumDistributor()->run();

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
        GetStatusBar()->SetStatusText(wxString::Format(wxT("Waterfall max speed changed to %d lines per second."),(int)ceil(val*val)));
    }

    wproc->setView(waterfallCanvas->getViewState());
    wproc->setBandwidth(waterfallCanvas->getBandwidth());
    wproc->setCenterFrequency(waterfallCanvas->getCenterFrequency());
    
    waterfallCanvas->processInputQueue();
    demodWaterfallCanvas->processInputQueue();

    if (!this->IsActive()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    event.RequestMore();
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
        *demod->newChild("stereo") = (*instance_i)->isStereo() ? 1 : 0;
        *demod->newChild("output_device") = outputDevices[(*instance_i)->getOutputDevice()].name;
        *demod->newChild("gain") = (*instance_i)->getGain();
        *demod->newChild("muted") = (*instance_i)->isMuted() ? 1 : 0;
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
            int type = demod->hasAnother("type") ? *demod->getNext("type") : DEMOD_TYPE_FM;
            float squelch_level = demod->hasAnother("squelch_level") ? (float) *demod->getNext("squelch_level") : 0;
            int squelch_enabled = demod->hasAnother("squelch_enabled") ? (int) *demod->getNext("squelch_enabled") : 0;
            int stereo = demod->hasAnother("stereo") ? (int) *demod->getNext("stereo") : 0;
            int muted = demod->hasAnother("muted") ? (int) *demod->getNext("muted") : 0;
            std::string output_device = demod->hasAnother("output_device") ? string(*(demod->getNext("output_device"))) : "";
            float gain = demod->hasAnother("gain") ? (float) *demod->getNext("gain") : 1.0;

            DemodulatorInstance *newDemod = wxGetApp().getDemodMgr().newThread();
            loadedDemod = newDemod;
            numDemodulators++;
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
            if (stereo) {
                newDemod->setStereo(true);
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
            std::cout << "\t\tStereo: " << (stereo ? "true" : "false") << std::endl;
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
