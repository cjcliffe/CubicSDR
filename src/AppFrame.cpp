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

    audioInputQueue = new AudioThreadInputQueue;
    audioThread = new AudioThread(audioInputQueue);

    threadAudio = new std::thread(&AudioThread::threadMain, audioThread);

    demodulatorTest = demodMgr.newThread();
    demodulatorTest->params.audioInputQueue = audioInputQueue;
    demodulatorTest->init();

    audioVisualQueue = new DemodulatorThreadOutputQueue();
    demodulatorTest->setVisualOutputQueue(audioVisualQueue);

    threadCmdQueueSDR = new SDRThreadCommandQueue;
    sdrThread = new SDRThread(threadCmdQueueSDR);
    sdrThread->bindDemodulator(demodulatorTest);

    iqVisualQueue = new SDRThreadIQDataQueue;
    sdrThread->setIQVisualQueue(iqVisualQueue);

    threadSDR = new std::thread(&SDRThread::threadMain, sdrThread);

//    static const int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
//    wxLogStatus("Double-buffered display %s supported", wxGLCanvas::IsDisplaySupported(attribs) ? "is" : "not");
//    ShowFullScreen(true);
}

AppFrame::~AppFrame() {

//    delete t_SDR;
    delete audioInputQueue;
    delete audioThread;
    delete threadCmdQueueSDR;
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

    std::vector<signed char> *new_uc_buffer;
    std::vector<float> *new_float_buffer;
    std::string asdf("beep");

    // SDR IQ -> Demodulator

    event.Skip();
}

void AppFrame::OnIdle(wxIdleEvent& event) {
    bool work_done = false;

    if (!iqVisualQueue->empty()) {
        SDRThreadIQData iqData;
        iqVisualQueue->pop(iqData);

        if (iqData.data.size()) {
            spectrumCanvas->setData(&iqData.data);
            waterfallCanvas->setData(&iqData.data);
        } else {
            std::cout << "Incoming IQ data empty?" << std::endl;
        }
        work_done = true;
    }

    if (!audioVisualQueue->empty()) {
        AudioThreadInput demodAudioData;
        audioVisualQueue->pop(demodAudioData);
        if (demodAudioData.data.size()) {

            if (scopeCanvas->waveform_points.size() != demodAudioData.data.size() * 2) {
                scopeCanvas->waveform_points.resize(demodAudioData.data.size() * 2);
            }

            for (int i = 0, iMax = demodAudioData.data.size(); i < iMax; i++) {
                scopeCanvas->waveform_points[i * 2 + 1] = demodAudioData.data[i] * 0.5f;
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

void AppFrame::setFrequency(unsigned int freq) {
    frequency = freq;
    SDRThreadCommand command(SDRThreadCommand::SDR_THREAD_CMD_TUNE);
    command.int_value = freq;
    threadCmdQueueSDR->push(command);
}

int AppFrame::getFrequency() {
    return frequency;
}
