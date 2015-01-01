#include "AppFrame.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include <vector>
#include "SDRThread.h"
#include "DemodulatorMgr.h"
#include "AudioThread.h"
#include "CubicSDR.h"

#include <thread>

#include <wx/panel.h>

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame)
//EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_MENU(wxID_CLOSE, AppFrame::OnClose)
EVT_MENU(wxID_ANY, AppFrame::OnMenu)

EVT_COMMAND(wxID_ANY, wxEVT_THREAD, AppFrame::OnThread)
EVT_IDLE(AppFrame::OnIdle)
wxEND_EVENT_TABLE()

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, wxT("CubicSDR")), activeDemodulator(NULL) {

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodOpts = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodVisuals = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodTray = new wxBoxSizer(wxHORIZONTAL);

    /*
     demodTray->AddSpacer(5);
     demodOpts->AddSpacer(5);

     wxStaticText *audioDeviceLabel = new wxStaticText(this, wxID_ANY, wxString("Audio Device:"));
     demodOpts->Add(audioDeviceLabel, 1, wxFIXED_MINSIZE | wxALL, 0);

     wxArrayString str;
     str.Add("Primary Device");
     wxChoice *wxCh = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, str);
     demodOpts->Add(wxCh, 1, wxFIXED_MINSIZE | wxALL, 0);

     demodOpts->AddSpacer(2);

     wxStaticText *demodTypeLabel = new wxStaticText(this, wxID_ANY, wxString("Demodulation:"));
     demodOpts->Add(demodTypeLabel, 1, wxFIXED_MINSIZE | wxALL, 0);

     str.Clear();
     str.Add("FM");
     str.Add("FM Stereo");
     str.Add("AM");
     str.Add("LSB");
     str.Add("USB");
     wxChoice *wxDemodChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, str);
     demodOpts->Add(wxDemodChoice, 1, wxFIXED_MINSIZE | wxALL, 0);

     demodOpts->AddSpacer(5);
     demodTray->AddSpacer(5);

     demodTray->Add(demodOpts, 1, wxEXPAND | wxALL, 0); */

    demodSpectrumCanvas = new SpectrumCanvas(this, NULL);
    demodSpectrumCanvas->Setup(1024);
    demodSpectrumCanvas->SetView(DEFAULT_FREQ, 300000);
    demodVisuals->Add(demodSpectrumCanvas, 1, wxEXPAND | wxALL, 0);

    demodVisuals->AddSpacer(1);

    demodWaterfallCanvas = new WaterfallCanvas(this, NULL);
    demodWaterfallCanvas->Setup(1024, 256);
    demodWaterfallCanvas->SetView(DEFAULT_FREQ, 300000);
    demodWaterfallCanvas->attachSpectrumCanvas(demodSpectrumCanvas);
    demodVisuals->Add(demodWaterfallCanvas, 3, wxEXPAND | wxALL, 0);

    demodTray->Add(demodVisuals, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(2);

    demodSignalMeter = new MeterCanvas(this, NULL);
    demodSignalMeter->setMax(0.5);
    demodTray->Add(demodSignalMeter, 1, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(2);

    scopeCanvas = new ScopeCanvas(this, NULL);
    demodTray->Add(scopeCanvas, 30, wxEXPAND | wxALL, 0);

    vbox->Add(demodTray, 2, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(2);
    spectrumCanvas = new SpectrumCanvas(this, NULL);
    spectrumCanvas->Setup(2048);
    vbox->Add(spectrumCanvas, 1, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(2);
    waterfallCanvas = new WaterfallCanvas(this, NULL);
    waterfallCanvas->Setup(2048, 512);
    waterfallCanvas->attachSpectrumCanvas(spectrumCanvas);
    vbox->Add(waterfallCanvas, 4, wxEXPAND | wxALL, 0);

    this->SetSizer(vbox);

    waterfallCanvas->SetFocusFromKbd();
    waterfallCanvas->SetFocus();

//    SetIcon(wxICON(sample));

// Make a menubar
//    wxMenu *menu = new wxMenu;
//    menu->Append(wxID_NEW);
//    menu->AppendSeparator();
//    menu->Append(wxID_CLOSE);
//    wxMenuBar *menuBar = new wxMenuBar;
//    menuBar->Append(menu, wxT("&File"));

    wxMenu *menu = new wxMenu;

    std::vector<RtAudio::DeviceInfo>::iterator devices_i;
    std::map<int, RtAudio::DeviceInfo>::iterator mdevices_i;
    AudioThread::enumerateDevices(devices);

    int i = 0;

    for (devices_i = devices.begin(); devices_i != devices.end(); devices_i++) {
        if (devices_i->inputChannels) {
            input_devices[i] = *devices_i;
        }
        if (devices_i->outputChannels) {
            output_devices[i] = *devices_i;
        }
        i++;
    }

    i = 0;

    for (mdevices_i = output_devices.begin(); mdevices_i != output_devices.end(); mdevices_i++) {
        wxMenuItem *itm = menu->AppendRadioItem(wxID_RT_AUDIO_DEVICE + mdevices_i->first, mdevices_i->second.name, wxT("Description?"));
        itm->SetId(wxID_RT_AUDIO_DEVICE + mdevices_i->first);
        if (mdevices_i->second.isDefaultOutput) {
            itm->Check(true);
        }
        output_device_menuitems[mdevices_i->first] = itm;
    }

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menu, wxT("Output &Device"));

    wxMenu *demodMenu = new wxMenu;
    demod_menuitems[DEMOD_TYPE_FM] = demodMenu->AppendRadioItem(wxID_DEMOD_TYPE_FM, wxT("FM"), wxT("Description?"));
    demod_menuitems[DEMOD_TYPE_AM] = demodMenu->AppendRadioItem(wxID_DEMOD_TYPE_AM, wxT("AM"), wxT("Description?"));
    demod_menuitems[DEMOD_TYPE_LSB] = demodMenu->AppendRadioItem(wxID_DEMOD_TYPE_LSB, wxT("LSB"), wxT("Description?"));
    demod_menuitems[DEMOD_TYPE_USB] = demodMenu->AppendRadioItem(wxID_DEMOD_TYPE_USB, wxT("USB"), wxT("Description?"));

    menuBar->Append(demodMenu, wxT("Demodulaton &Type"));

    SetMenuBar(menuBar);

    CreateStatusBar();
    SetClientSize(1280, 600);
    Centre();
    Show();

    GetStatusBar()->SetStatusText(wxString::Format(wxT("Set center frequency: %i"), DEFAULT_FREQ));

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
    }

    if (activeDemodulator) {
        if (event.GetId() == wxID_DEMOD_TYPE_FM) {
            activeDemodulator->setDemodulatorType(DEMOD_TYPE_FM);
            activeDemodulator = NULL;
        } else if (event.GetId() == wxID_DEMOD_TYPE_AM) {
            activeDemodulator->setDemodulatorType(DEMOD_TYPE_AM);
            activeDemodulator = NULL;
        } else if (event.GetId() == wxID_DEMOD_TYPE_LSB) {
            activeDemodulator->setDemodulatorType(DEMOD_TYPE_LSB);
            activeDemodulator = NULL;
        } else if (event.GetId() == wxID_DEMOD_TYPE_USB) {
            activeDemodulator->setDemodulatorType(DEMOD_TYPE_USB);
            activeDemodulator = NULL;
        }
    }
}

void AppFrame::OnClose(wxCommandEvent& WXUNUSED(event)) {
    Close(true);
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
        if (demod != activeDemodulator) {
            demodSignalMeter->setInputValue(demod->getSquelchLevel());
            int outputDevice = demod->getOutputDevice();
            scopeCanvas->setDeviceName(output_devices[outputDevice].name);
            output_device_menuitems[outputDevice]->Check(true);
            int dType = demod->getDemodulatorType();
            demod_menuitems[dType]->Check(true);
        }
        if (demodWaterfallCanvas->getDragState() == WaterfallCanvas::WF_DRAG_NONE) {
            if (demod->getParams().frequency != demodWaterfallCanvas->GetCenterFrequency()) {
                demodWaterfallCanvas->SetCenterFrequency(demod->getParams().frequency);
                demodSpectrumCanvas->SetCenterFrequency(demod->getParams().frequency);
            }
            unsigned int demodBw = (unsigned int) ceil((float) demod->getParams().bandwidth * 2.5);
            if (demodBw > SRATE / 2) {
                demodBw = SRATE / 2;
            }
            if (demodBw < 80000) {
                demodBw = 80000;
            }
            demodWaterfallCanvas->SetBandwidth(demodBw);
            demodSpectrumCanvas->SetBandwidth(demodBw);
        }
        demodSignalMeter->setLevel(demod->getSignalLevel());
        if (demodSignalMeter->inputChanged()) {
            demod->setSquelchLevel(demodSignalMeter->getInputValue());
        }
        activeDemodulator = demod;
    }

    if (!wxGetApp().getIQVisualQueue()->empty()) {
        DemodulatorThreadIQData *iqData;
        wxGetApp().getIQVisualQueue()->pop(iqData);

        if (iqData && iqData->data.size()) {
//            spectrumCanvas->setData(iqData);
            waterfallCanvas->setData(iqData);
            demodWaterfallCanvas->setData(iqData);
            delete iqData;
        } else {
            std::cout << "Incoming IQ data empty?" << std::endl;
        }
        work_done = true;
    }

    if (!wxGetApp().getAudioVisualQueue()->empty()) {
        AudioThreadInput *demodAudioData;
        wxGetApp().getAudioVisualQueue()->pop(demodAudioData);
        if (demodAudioData && demodAudioData->data.size()) {
            if (scopeCanvas->waveform_points.size() != demodAudioData->data.size() * 2) {
                scopeCanvas->waveform_points.resize(demodAudioData->data.size() * 2);
            }

            for (int i = 0, iMax = demodAudioData->data.size(); i < iMax; i++) {
                scopeCanvas->waveform_points[i * 2 + 1] = demodAudioData->data[i] * 0.5f;
                scopeCanvas->waveform_points[i * 2] = ((double) i / (double) iMax);
            }

            scopeCanvas->setStereo(demodAudioData->channels == 2);

            delete demodAudioData;
        } else {
            std::cout << "Incoming Demodulator data empty?" << std::endl;
        }
        work_done = true;
    }

    if (!work_done) {
        event.Skip();
    }
}
