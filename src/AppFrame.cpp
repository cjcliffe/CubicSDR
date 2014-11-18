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

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame)
//EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_MENU(wxID_CLOSE, AppFrame::OnClose)
EVT_COMMAND(wxID_ANY, wxEVT_THREAD, AppFrame::OnThread)
EVT_IDLE(AppFrame::OnIdle)
wxEND_EVENT_TABLE()

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, wxT("CubicSDR")), frequency(DEFAULT_FREQ) {

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

    threadQueueSDR = new SDRThreadQueue(this);
    t_SDR = new SDRThread(threadQueueSDR);
    if (t_SDR->Run() != wxTHREAD_NO_ERROR) {
        wxLogError
        ("Can't create the SDR thread!");
        delete t_SDR;
        t_SDR = NULL;
    }

    threadQueueAudio = new AudioThreadQueue(this);
    t_Audio = new AudioThread(threadQueueAudio);
    if (t_Audio->Run() != wxTHREAD_NO_ERROR) {
        wxLogError
        ("Can't create the Audio thread!");
        delete t_Audio;
        t_Audio = NULL;
    }

    demodulatorTest = demodMgr.newThread(this);
    demodulatorTest->params.audioQueue = threadQueueAudio;
    demodulatorTest->run();

    t_SDR->bindDemodulator(*demodulatorTest);
//    static const int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
//    wxLogStatus("Double-buffered display %s supported", wxGLCanvas::IsDisplaySupported(attribs) ? "is" : "not");
//    ShowFullScreen(true);
}

AppFrame::~AppFrame() {
    {
        wxCriticalSectionLocker enter(m_pThreadCS);
        if (t_SDR) {
            wxMessageOutputDebug().Printf("CubicSDR: deleting thread");
            if (t_SDR->Delete() != wxTHREAD_NO_ERROR) {
                wxLogError
                ("Can't delete the thread!");
            }
        }
    }

//    delete t_SDR;
    delete threadQueueAudio;
    delete threadQueueSDR;
}

void AppFrame::OnClose(wxCommandEvent& WXUNUSED(event)) {

    // true is to force the frame to close
    Close(true);
}

void AppFrame::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
    new AppFrame();
}

void AppFrame::OnThread(wxCommandEvent& event) {
    SDRThreadIQData *iqData;
    DemodulatorThreadAudioData *demodAudioData;

    std::vector<signed char> *new_uc_buffer;
    std::vector<float> *new_float_buffer;

    switch (event.GetId()) {

    // SDR IQ -> Demodulator
    case SDRThreadTask::SDR_THREAD_DATA:
        iqData = (SDRThreadIQData *) event.GetClientData();
        new_uc_buffer = &(iqData->data);
        if (new_uc_buffer->size()) {
            spectrumCanvas->setData(new_uc_buffer);
            waterfallCanvas->setData(new_uc_buffer);
        } else {
            std::cout << "Incoming IQ data empty?" << std::endl;
        }
        delete iqData;

        break; // thread wants to exit: disable controls and destroy main window

        // Demodulator -> Audio
    case DemodulatorThreadTask::DEMOD_THREAD_AUDIO_DATA:
        demodAudioData = (DemodulatorThreadAudioData *) event.GetClientData();
        new_float_buffer = &(demodAudioData->data);
        if (new_float_buffer != NULL && new_float_buffer->size()) {

            if (scopeCanvas->waveform_points.size() != new_float_buffer->size() * 2) {
                scopeCanvas->waveform_points.resize(new_float_buffer->size() * 2);
            }

            for (int i = 0, iMax = new_float_buffer->size(); i < iMax; i++) {
                scopeCanvas->waveform_points[i * 2 + 1] = (*new_float_buffer)[i] * 0.5f;
                scopeCanvas->waveform_points[i * 2] = ((double) i / (double) iMax);
            }

        } else {
            std::cout << "Incoming Demodulator data empty?" << std::endl;
        }
        delete demodAudioData;

        break;
    default:
        event.Skip();
    }
}

void AppFrame::OnIdle(wxIdleEvent& event) {

    event.Skip();
}

void AppFrame::setFrequency(unsigned int freq) {
    frequency = freq;
    SDRThreadTask task = SDRThreadTask(SDRThreadTask::SDR_THREAD_TUNING);
    task.setUInt(freq);
    threadQueueSDR->addTask(task, SDRThreadQueue::SDR_PRIORITY_HIGHEST);
}

int AppFrame::getFrequency() {
    return frequency;
}
