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

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame)
//EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_MENU(wxID_CLOSE, AppFrame::OnClose)
EVT_COMMAND(wxID_ANY, wxEVT_THREAD, AppFrame::OnThread)
EVT_IDLE(AppFrame::OnIdle)
wxEND_EVENT_TABLE()

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, wxT("CubicSDR")) {

    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);

    scopeCanvas = new ScopeCanvas(this, NULL);
    vbox->Add(scopeCanvas, 1, wxEXPAND | wxALL, 0);
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


    GetStatusBar()->SetStatusText(wxString::Format(wxT("Set center frequency: %i"),DEFAULT_FREQ));


//    static const int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
//    wxLogStatus("Double-buffered display %s supported", wxGLCanvas::IsDisplaySupported(attribs) ? "is" : "not");
//    ShowFullScreen(true);
}

AppFrame::~AppFrame() {

//    delete t_SDR;

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
        SDRThreadIQData iqData;
        wxGetApp().getIQVisualQueue()->pop(iqData);

        if (iqData.data.size()) {
            spectrumCanvas->setData(&iqData.data);
            waterfallCanvas->setData(&iqData.data);
        } else {
            std::cout << "Incoming IQ data empty?" << std::endl;
        }
        work_done = true;
    }

    if (!wxGetApp().getAudioVisualQueue()->empty()) {
        AudioThreadInput demodAudioData;
        wxGetApp().getAudioVisualQueue()->pop(demodAudioData);
        if (demodAudioData.data.size()) {

            if (scopeCanvas->waveform_points.size() != demodAudioData.data.size()) {
                scopeCanvas->waveform_points.resize(demodAudioData.data.size());
            }

            for (int i = 0, iMax = demodAudioData.data.size()/2; i < iMax; i++) {
                scopeCanvas->waveform_points[i * 2 + 1] = demodAudioData.data[i*2] * 0.5f;
                scopeCanvas->waveform_points[i * 2] = ((double) i / (double) iMax);
            }

        } else {
            std::cout << "Incoming Demodulator data empty?" << std::endl;
        }
        work_done = true;
    }

    if (!work_done) {
    	event.Skip();
    }
}
