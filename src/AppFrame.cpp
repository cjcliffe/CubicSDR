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
wxFrame(NULL, wxID_ANY, wxT("CubicSDR v0.1a by Charles J. Cliffe (@ccliffe)")), activeDemodulator(NULL) {

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodOpts = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodVisuals = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodTray = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *demodScopeTray = new wxBoxSizer(wxVERTICAL);

    demodModeSelector = new ModeSelectorCanvas(this, NULL);
    demodModeSelector->addChoice(DEMOD_TYPE_FM,"FM");
    demodModeSelector->addChoice(DEMOD_TYPE_AM,"AM");
    demodModeSelector->addChoice(DEMOD_TYPE_LSB,"LSB");
    demodModeSelector->addChoice(DEMOD_TYPE_USB,"USB");
    demodModeSelector->addChoice(DEMOD_TYPE_DSB,"DSB");
    demodTray->Add(demodModeSelector, 2, wxEXPAND | wxALL, 0);

//    demodTray->AddSpacer(2);

    demodSpectrumCanvas = new SpectrumCanvas(this, NULL);
    demodSpectrumCanvas->setup(1024);
    demodSpectrumCanvas->setView(DEFAULT_FREQ, 300000);
    demodVisuals->Add(demodSpectrumCanvas, 3, wxEXPAND | wxALL, 0);

    demodVisuals->AddSpacer(1);

    demodWaterfallCanvas = new WaterfallCanvas(this, NULL);
    demodWaterfallCanvas->setup(1024, 128);
    demodWaterfallCanvas->setView(DEFAULT_FREQ, 300000);
    demodWaterfallCanvas->attachSpectrumCanvas(demodSpectrumCanvas);
    demodSpectrumCanvas->attachWaterfallCanvas(demodWaterfallCanvas);
    demodVisuals->Add(demodWaterfallCanvas, 6, wxEXPAND | wxALL, 0);

    demodTray->Add(demodVisuals, 30, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(2);

    demodSignalMeter = new MeterCanvas(this, NULL);
    demodSignalMeter->setMax(0.5);
    demodSignalMeter->setHelpTip("Current Signal Level.  Click / Drag to set Squelch level.");
    demodTray->Add(demodSignalMeter, 1, wxEXPAND | wxALL, 0);

    demodTray->AddSpacer(2);

    scopeCanvas = new ScopeCanvas(this, NULL);
    demodScopeTray->Add(scopeCanvas, 8, wxEXPAND | wxALL, 0);

    demodScopeTray->AddSpacer(2);

    demodTuner = new TuningCanvas(this, NULL);
    demodTuner->setHelpTip("Testing tuner");
    demodScopeTray->Add(demodTuner, 1, wxEXPAND | wxALL, 0);

    demodTray->Add(demodScopeTray, 30, wxEXPAND | wxALL, 0);

    vbox->Add(demodTray, 12, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(2);
    spectrumCanvas = new SpectrumCanvas(this, NULL);
    spectrumCanvas->setup(2048);
    vbox->Add(spectrumCanvas, 5, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(2);
    waterfallCanvas = new WaterfallCanvas(this, NULL);
    waterfallCanvas->setup(2048, 512);
    waterfallCanvas->attachSpectrumCanvas(spectrumCanvas);
    spectrumCanvas->attachWaterfallCanvas(waterfallCanvas);
    vbox->Add(waterfallCanvas, 20, wxEXPAND | wxALL, 0);

    this->SetSizer(vbox);

//    waterfallCanvas->SetFocusFromKbd();
    waterfallCanvas->SetFocus();

//    SetIcon(wxICON(sample));

// Make a menubar
    wxMenuBar *menuBar = new wxMenuBar;
    wxMenu *menu = new wxMenu;
//    menu->Append(wxID_NEW);
    menu->Append(wxID_SET_FREQ_OFFSET, "Set Frequency Offset");
    menu->Append(wxID_OPEN, "&Open Session");
    menu->Append(wxID_SAVE, "&Save Session");
    menu->Append(wxID_SAVEAS, "Save Session &As..");
    menu->AppendSeparator();
    menu->Append(wxID_CLOSE);

    menuBar->Append(menu, wxT("&File"));

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

    i = 0;

    for (mdevices_i = outputDevices.begin(); mdevices_i != outputDevices.end(); mdevices_i++) {
        wxMenuItem *itm = menu->AppendRadioItem(wxID_RT_AUDIO_DEVICE + mdevices_i->first, mdevices_i->second.name, wxT("Description?"));
        itm->SetId(wxID_RT_AUDIO_DEVICE + mdevices_i->first);
        if (mdevices_i->second.isDefaultOutput) {
            itm->Check(true);
        }
        outputDeviceMenuItems[mdevices_i->first] = itm;
    }

    menuBar->Append(menu, wxT("Active Demodulator &Output"));

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
    } else if (event.GetId() == wxID_SET_FREQ_OFFSET) {
        long ofs = wxGetNumberFromUser("Shift the displayed frequency by this amount.\ni.e. -125000000 for -125 MHz", "Frequency (Hz)",
                "Frequency Offset", wxGetApp().getOffset(), -2000000000, 2000000000, this);
        if (ofs != -1) {
            wxGetApp().setOffset(ofs);
        }
    } else if (event.GetId() == wxID_SAVE) {
        wxFileDialog saveFileDialog(this, _("Save XML Session file"), "", "", "XML files (*.xml)|*.xml", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
       if (saveFileDialog.ShowModal() == wxID_CANCEL) {
           return;
       }
       // saveFileDialog.GetPath();
    } else if (event.GetId() == wxID_OPEN) {
        wxFileDialog openFileDialog(this, _("Open XML Session file"), "", "","XML files (*.xml)|*.xml", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
       if (openFileDialog.ShowModal() == wxID_CANCEL) {
           return;
       }
       // openFileDialog.GetPath();
    } else if (event.GetId() == wxID_SAVEAS) {
    } else if (event.GetId() == wxID_EXIT) {
        Close(false);
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
        if (demod != activeDemodulator) {
            demodSignalMeter->setInputValue(demod->getSquelchLevel());
            int outputDevice = demod->getOutputDevice();
            scopeCanvas->setDeviceName(outputDevices[outputDevice].name);
            outputDeviceMenuItems[outputDevice]->Check(true);
            int dType = demod->getDemodulatorType();
            demodModeSelector->setSelection(dType);
        }
        if (demodWaterfallCanvas->getDragState() == WaterfallCanvas::WF_DRAG_NONE) {
            if (demod->getParams().frequency != demodWaterfallCanvas->getCenterFrequency()) {
                demodWaterfallCanvas->setCenterFrequency(demod->getFrequency());
                demodSpectrumCanvas->setCenterFrequency(demod->getFrequency());
            }
            int dSelection = demodModeSelector->getSelection();
            if (dSelection != -1 && dSelection != demod->getDemodulatorType()) {
                demod->setDemodulatorType(dSelection);
            }

            unsigned int demodBw = (unsigned int) ceil((float) demod->getParams().bandwidth * 2.5);
            if (demodBw > SRATE / 2) {
                demodBw = SRATE / 2;
            }
            if (demodBw < 80000) {
                demodBw = 80000;
            }
            demodWaterfallCanvas->setBandwidth(demodBw);
            demodSpectrumCanvas->setBandwidth(demodBw);
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

    if (!waterfallCanvas->HasFocus()) {
        waterfallCanvas->SetFocus();
    }

    if (!work_done) {
        event.Skip();
    }
}
