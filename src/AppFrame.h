#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"
#include "SDRThread.h"
#include "AudioThread.h"
#include "DemodulatorMgr.h"

#include "ScopeCanvas.h"
#include "SpectrumCanvas.h"
#include "WaterfallCanvas.h"


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

    SDRThread *t_SDR;
    SDRThreadQueue* threadQueueSDR;
    AudioThread *t_Audio;
    AudioThreadQueue* threadQueueAudio;
    DemodulatorMgr demodMgr;

    wxCriticalSection m_pThreadCS;
    unsigned int frequency;

    DemodulatorInstance *demodulatorTest;

// event table
wxDECLARE_EVENT_TABLE();
};
