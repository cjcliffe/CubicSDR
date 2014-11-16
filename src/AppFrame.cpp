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
#include "DemodulatorThread.h"
#include "AudioThread.h"

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame)
//EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_MENU(wxID_CLOSE, AppFrame::OnClose)
EVT_THREAD(EVENT_SDR_INPUT, AppFrame::OnEventInput)
EVT_THREAD(EVENT_DEMOD_INPUT, AppFrame::OnDemodInput)
EVT_THREAD(EVENT_AUDIO_INPUT, AppFrame::OnAudioInput)
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
    SetClientSize(1280, 400);
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

    threadQueueDemod = new DemodulatorThreadQueue(this);
    t_Demod = new DemodulatorThread(threadQueueDemod);
    if (t_Demod->Run() != wxTHREAD_NO_ERROR) {
        wxLogError
        ("Can't create the Demodulator thread!");
        delete t_Demod;
        t_Demod = NULL;
    }

    threadQueueAudio = new AudioThreadQueue(this);
    t_Audio = new AudioThread(threadQueueAudio);
    if (t_Audio->Run() != wxTHREAD_NO_ERROR) {
        wxLogError
        ("Can't create the Audio thread!");
        delete t_Audio;
        t_Audio = NULL;
    }

//    t_IQBuffer = new IQBufferThread(this);
//    if (t_IQBuffer->Run() != wxTHREAD_NO_ERROR) {
//        wxLogError
//        ("Can't create the thread!");
//        delete t_IQBuffer;
//    t_IQBuffer = NULL;
//    }

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

//    {
//        wxCriticalSectionLocker enter(m_pThreadCS);
//        if (t_IQBuffer) {
//            wxMessageOutputDebug().Printf("CubicSDR: deleting thread");
//            if (t_IQBuffer->Delete() != wxTHREAD_NO_ERROR) {
//                wxLogError
//                ("Can't delete the thread!");
//            }
//        }
//    }

//    delete t_SDR;
//    delete t_IQBuffer;
    delete threadQueueAudio;
    delete threadQueueDemod;
    delete threadQueueSDR;
}

void AppFrame::OnClose(wxCommandEvent& WXUNUSED(event)) {

    // true is to force the frame to close
    Close(true);
}

void AppFrame::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
    new AppFrame();
}

// SDR IQ -> Demodulator
void AppFrame::OnEventInput(wxThreadEvent& event) {
    std::vector<signed char> *new_buffer = event.GetPayload<std::vector<signed char> *>();
//    std::cout << new_buffer->size() << std::endl;
    if (new_buffer->size()) {
        test_demod.writeBuffer(new_buffer);
        scopeCanvas->setWaveformPoints(test_demod.waveform_points);
        spectrumCanvas->setData(new_buffer);
        waterfallCanvas->setData(new_buffer);
    } else {
        std::cout << "Incoming IQ data empty?" << std::endl;
    }
    delete new_buffer;
}


// Demodulator -> Audio
void AppFrame::OnDemodInput(wxThreadEvent& event) {
    std::vector<float> *new_buffer = event.GetPayload<std::vector<float> *>();

    if (new_buffer->size()) {
        AudioThreadTask task = AudioThreadTask(AudioThreadTask::AUDIO_THREAD_DATA);
        task.setData(*new_buffer);
        threadQueueAudio->addTask(task, AudioThreadQueue::AUDIO_PRIORITY_HIGHEST);
    } else {
        std::cout << "Incoming Demod data empty?" << std::endl;
    }
    delete new_buffer;
}

// Audio -> Visual
void AppFrame::OnAudioInput(wxThreadEvent& event) {
//    std::vector<float> *new_buffer = event.GetPayload<std::vector<float> *>();
//
//    if (new_buffer->size()) {
//        AudioThreadTask task = AudioThreadTask(AudioThreadTask::AUDIO_THREAD_DATA);
//        task.setData(*new_buffer);
//        threadQueueAudio->addTask(task, AudioThreadQueue::AUDIO_PRIORITY_HIGHEST);
//    } else {
//        std::cout << "Incoming Demod data empty?" << std::endl;
//    }
//    delete new_buffer;
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
