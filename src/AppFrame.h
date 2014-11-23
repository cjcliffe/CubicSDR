#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"
#include "SDRThread.h"
#include "AudioThread.h"
#include "DemodulatorMgr.h"

#include "ScopeCanvas.h"
#include "SpectrumCanvas.h"
#include "WaterfallCanvas.h"
#include "ThreadQueue.h"

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();
    void OnThread (wxCommandEvent& event);
    void OnEventInput(wxThreadEvent& event);


    void setFrequency(unsigned int freq);
    int getFrequency();

private:
    void OnClose(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);

    ScopeCanvas *scopeCanvas;
    SpectrumCanvas *spectrumCanvas;
    WaterfallCanvas *waterfallCanvas;

    DemodulatorMgr demodMgr;

    wxCriticalSection m_pThreadCS;
    unsigned int frequency;

    DemodulatorInstance *demodulatorTest;

    AudioThreadInputQueue *audioInputQueue;
    AudioThread *audioThread;

    SDRThread *sdrThread;
    SDRThreadCommandQueue* threadCmdQueueSDR;
    SDRThreadIQDataQueue* iqVisualQueue;
    DemodulatorThreadOutputQueue* audioVisualQueue;

    std::thread *t1;
    std::thread *t_SDR;

// event table
wxDECLARE_EVENT_TABLE();
};
