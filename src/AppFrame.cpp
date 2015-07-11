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
EVT_MENU(wxID_CLOSE, AppFrame::OnClose)
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
    demodModeSelector->setHelpTip("Choose modulation type: Frequency Modulation, Amplitude Modulation and Lower, Upper or Double Side-Band.");
    demodTray->Add(demodModeSelector, 2, wxEXPAND | wxALL, 0);
    
    demodModeSelectorAdv = new ModeSelectorCanvas(this, attribList);
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_ASK, "ASK");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_APSK, "APSK");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_BPSK, "BPSK");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_DPSK, "DPSK");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_PSK, "PSK");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_OOK, "OOK");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_ST, "ST");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_SQAM, "SQAM");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_QAM, "QAM");
    demodModeSelectorAdv->addChoice(DEMOD_TYPE_QPSK, "QPSK");
    demodModeSelectorAdv->setSelection(DEMOD_TYPE_ASK);
    demodModeSelectorAdv->setHelpTip("Choose advanced modulation types.");
    demodTray->Add(demodModeSelectorAdv, 3, wxEXPAND | wxALL, 0);
    
    demodModeSelectorCons = new ModeSelectorCanvas(this, attribList);
    demodModeSelectorCons->addChoice(1, "auto");
    demodModeSelectorCons->addChoice(2, "2");
    demodModeSelectorCons->addChoice(4, "4");
    demodModeSelectorCons->addChoice(8, "8");
    demodModeSelectorCons->addChoice(16, "16");
    demodModeSelectorCons->addChoice(32, "32");
    demodModeSelectorCons->addChoice(64, "64");
    demodModeSelectorCons->addChoice(128, "128");
    demodModeSelectorCons->addChoice(256, "256");
    demodModeSelectorCons->setHelpTip("Choose number of constallations types.");
    demodTray->Add(demodModeSelectorCons, 2, wxEXPAND | wxALL, 0);

//    demodTray->AddSpacer(2);

    demodSpectrumCanvas = new SpectrumCanvas(this, attribList);
    demodSpectrumCanvas->setup(1024);
    demodSpectrumCanvas->setView(DEFAULT_FREQ, 300000);
    demodVisuals->Add(demodSpectrumCanvas, 3, wxEXPAND | wxALL, 0);

    demodVisuals->AddSpacer(1);

    demodWaterfallCanvas = new WaterfallCanvas(this, attribList);
    demodWaterfallCanvas->setup(1024, 128);
    demodWaterfallCanvas->setView(DEFAULT_FREQ, 300000);
    demodWaterfallCanvas->attachSpectrumCanvas(demodSpectrumCanvas);
    demodSpectrumCanvas->attachWaterfallCanvas(demodWaterfallCanvas);
    demodVisuals->Add(demodWaterfallCanvas, 6, wxEXPAND | wxALL, 0);

    demodTray->Add(demodVisuals, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(1);

    demodSignalMeter = new MeterCanvas(this, attribList);
    demodSignalMeter->setMax(0.5);
    demodSignalMeter->setHelpTip("Current Signal Level.  Click / Drag to set Squelch level.");
    demodTray->Add(demodSignalMeter, 1, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(1);

    scopeCanvas = new ScopeCanvas(this, attribList);
    demodScopeTray->Add(scopeCanvas, 8, wxEXPAND | wxALL, 0);

    demodScopeTray->AddSpacer(1);

    demodTuner = new TuningCanvas(this, attribList);
    demodTuner->setHelpTip("Testing tuner");
    demodScopeTray->Add(demodTuner, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodScopeTray, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(1);

    demodGainMeter = new MeterCanvas(this, attribList);
    demodGainMeter->setMax(2.0);
    demodGainMeter->setHelpTip("Current Demodulator Gain Level.  Click / Drag to set Gain level.");
    demodTray->Add(demodGainMeter, 1, wxEXPAND | wxALL, 0);

    vbox->Add(demodTray, 12, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(1);
    spectrumCanvas = new SpectrumCanvas(this, attribList);
    spectrumCanvas->setup(2048);
    vbox->Add(spectrumCanvas, 5, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(1);
    waterfallCanvas = new WaterfallCanvas(this, attribList);
    waterfallCanvas->setup(2048, 512);
    waterfallCanvas->attachSpectrumCanvas(spectrumCanvas);
    waterfallCanvas->attachWaterfallCanvas(demodWaterfallCanvas);
    spectrumCanvas->attachWaterfallCanvas(waterfallCanvas);
    vbox->Add(waterfallCanvas, 20, wxEXPAND | wxALL, 0);

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
    
    dsMenu->AppendRadioItem(wxID_SET_DS_OFF, "Off");
    dsMenu->AppendRadioItem(wxID_SET_DS_I, "I-ADC");
    dsMenu->AppendRadioItem(wxID_SET_DS_Q, "Q-ADC");
    
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

    menu->AppendRadioItem(wxID_THEME_DEFAULT, "Default")->Check(true);
    menu->AppendRadioItem(wxID_THEME_RADAR, "RADAR");
    menu->AppendRadioItem(wxID_THEME_BW, "Black & White");
    menu->AppendRadioItem(wxID_THEME_SHARP, "Sharp");
    menu->AppendRadioItem(wxID_THEME_RAD, "Rad");
    menu->AppendRadioItem(wxID_THEME_TOUCH, "Touch");
    menu->AppendRadioItem(wxID_THEME_HD, "HD");

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
    SetClientSize(1280, 600);
    Centre();
    Show();

#ifdef _WIN32
    SetIcon(wxICON(frame_icon));
#endif
    GetStatusBar()->SetStatusText(wxString::Format(wxT("Set center frequency: %i"), DEFAULT_FREQ));

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
        }
    } else if (event.GetId() == wxID_SET_DS_OFF) {
        wxGetApp().setDirectSampling(0);
    } else if (event.GetId() == wxID_SET_DS_I) {
        wxGetApp().setDirectSampling(1);
    } else if (event.GetId() == wxID_SET_DS_Q) {
        wxGetApp().setDirectSampling(2);
    } else if (event.GetId() == wxID_SET_SWAP_IQ) {
        bool swap_state = !wxGetApp().getSwapIQ();
        wxGetApp().setSwapIQ(swap_state);
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
        wxGetApp().setFrequency(DEFAULT_FREQ);
        wxGetApp().setOffset(0);
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
        wxGetApp().setDevice(event.GetId() - wxID_DEVICE_ID);
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

void AppFrame::OnClose(wxCommandEvent& WXUNUSED(event)) {
    Close(false);
}

void AppFrame::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
    new AppFrame();
}

void AppFrame::OnThread(wxCommandEvent& event) {
    event.Skip();
}

void AppFrame::OnIdle(wxIdleEvent& event) {
    bool work_done = false;

//#ifdef __APPLE__
//    std::this_thread::sleep_for(std::chrono::milliseconds(4));
//    std::this_thread::yield();
//#endif

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
            int dCons = demod->getDemodulatorCons();
            demodModeSelector->setSelection(dType);
            demodModeSelectorAdv->setSelection(dType);
            demodModeSelectorCons->setSelection(dCons);
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
            int dSelectionadv = demodModeSelectorAdv->getSelection();
			int dSelectionCons = demodModeSelectorCons->getSelection();

            // basic demodulators
            if (dSelection != -1 && dSelection != demod->getDemodulatorType()) {
                demod->setDemodulatorType(dSelection);
                demodModeSelectorAdv->setSelection(-1);
            } 
            // advanced demodulators
			else if (dSelectionadv != -1 && dSelectionadv != demod->getDemodulatorType()) {
				demod->setDemodulatorType(dSelectionadv);
				demodModeSelector->setSelection(-1);
            }

			// set constellations
			if (dSelectionCons != demod->getDemodulatorCons()) {
				demod->setDemodulatorCons(dSelectionCons);
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
        int dSelectionadv = demodModeSelectorAdv->getSelection();
		int dSelectionCons = demodModeSelectorCons->getSelection();

        // basic demodulators
        if (dSelection != -1 && dSelection != mgr->getLastDemodulatorType()) {
            mgr->setLastDemodulatorType(dSelection);
            demodModeSelectorAdv->setSelection(-1);
        }
        // advanced demodulators
        else if(dSelectionadv != -1 && dSelectionadv != mgr->getLastDemodulatorType()) {
            mgr->setLastDemodulatorType(dSelectionadv);
            demodModeSelector->setSelection(-1);
        }

		// set constellations
		if (dSelectionCons != mgr->getLastDemodulatorCons()) {
			mgr->setLastDemodulatorCons(dSelectionCons);
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

    event.Skip();
}

void AppFrame::saveSession(std::string fileName) {
    DataTree s("cubicsdr_session");
    DataNode *header = s.rootNode()->newChild("header");
    *header->newChild("version") = std::string(CUBICSDR_VERSION);
    *header->newChild("center_freq") = wxGetApp().getFrequency();
    *header->newChild("offset") = wxGetApp().getOffset();

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
        long long offset = *header->getNext("offset");

        std::cout << "Loading " << version << " session file" << std::endl;
        std::cout << "\tCenter Frequency: " << center_freq << std::endl;
        std::cout << "\tOffset: " << offset << std::endl;

        wxGetApp().setFrequency(center_freq);
        wxGetApp().setOffset(offset);

        DataNode *demodulators = l.rootNode()->getNext("demodulators");

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
            std::string output_device = demod->hasAnother("output_device") ? string(*(demod->getNext("output_device"))) : "";
            float gain = demod->hasAnother("gain") ? (float) *demod->getNext("gain") : 1.0;

            DemodulatorInstance *newDemod = wxGetApp().getDemodMgr().newThread();
            newDemod->setDemodulatorType(type);
            newDemod->setBandwidth(bandwidth);
            newDemod->setFrequency(freq);
            newDemod->setGain(gain);
            newDemod->updateLabel(freq);
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

            wxGetApp().bindDemodulator(newDemod);

            std::cout << "\tAdded demodulator at frequency " << freq << " type " << type << std::endl;
            std::cout << "\t\tBandwidth: " << bandwidth << std::endl;
            std::cout << "\t\tSquelch Level: " << squelch_level << std::endl;
            std::cout << "\t\tSquelch Enabled: " << (squelch_enabled ? "true" : "false") << std::endl;
            std::cout << "\t\tStereo: " << (stereo ? "true" : "false") << std::endl;
            std::cout << "\t\tOutput Device: " << output_device << std::endl;
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
