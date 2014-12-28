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
EVT_COMMAND(wxID_ANY, wxEVT_THREAD, AppFrame::OnThread)
EVT_IDLE(AppFrame::OnIdle)
wxEND_EVENT_TABLE()

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, wxT("CubicSDR")) {

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *demodTray = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *demodOpts = new wxBoxSizer(wxVERTICAL);

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

    demodTray->Add(demodOpts, 1, wxEXPAND | wxALL, 0);

    scopeCanvas = new ScopeCanvas(this, NULL);
    demodTray->Add(scopeCanvas, 7, wxEXPAND | wxALL, 0);

    vbox->Add(demodTray, 1, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(2);
    spectrumCanvas = new SpectrumCanvas(this, NULL);
    vbox->Add(spectrumCanvas, 1, wxEXPAND | wxALL, 0);
    vbox->AddSpacer(2);
    waterfallCanvas = new WaterfallCanvas(this, NULL);
    vbox->Add(waterfallCanvas, 4, wxEXPAND | wxALL, 0);

    this->SetSizer(vbox);

    waterfallCanvas->SetFocusFromKbd();
    waterfallCanvas->SetFocus();

//    SetIcon(wxICON(sample));

// Make a menubar
    wxMenu *menu = new wxMenu;
//    menu->Append(wxID_NEW);
//    menu->AppendSeparator();
    menu->Append(wxID_CLOSE);
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menu, wxT("&File"));

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

void AppFrame::OnClose(wxCommandEvent& WXUNUSED(event)) {

    // true is to force the frame to close
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
    if (!wxGetApp().getIQVisualQueue()->empty()) {
        DemodulatorThreadIQData *iqData;
        wxGetApp().getIQVisualQueue()->pop(iqData);

        if (iqData && iqData->data.size()) {
            spectrumCanvas->setData(&iqData->data);
            waterfallCanvas->setData(&iqData->data);

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
            if (scopeCanvas->waveform_points.size() != demodAudioData->data.size()*2) {
                scopeCanvas->waveform_points.resize(demodAudioData->data.size()*2);
            }

            for (int i = 0, iMax = demodAudioData->data.size(); i < iMax; i++) {
                scopeCanvas->waveform_points[i * 2 + 1] = demodAudioData->data[i] * 0.5f;
                scopeCanvas->waveform_points[i * 2] = ((double) i / (double) iMax);
            }

            scopeCanvas->setDivider(demodAudioData->channels == 2);

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
