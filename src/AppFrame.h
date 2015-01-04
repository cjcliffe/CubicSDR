#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"

#include "ScopeCanvas.h"
#include "SpectrumCanvas.h"
#include "WaterfallCanvas.h"
#include "MeterCanvas.h"
#include "TuningCanvas.h"

#include <map>

#define wxID_RT_AUDIO_DEVICE 1000
#define wxID_DEMOD_TYPE_FM 2000
#define wxID_DEMOD_TYPE_AM 2001
#define wxID_DEMOD_TYPE_LSB 2002
#define wxID_DEMOD_TYPE_USB 2003

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();
    void OnThread(wxCommandEvent& event);
    void OnEventInput(wxThreadEvent& event);

private:
    void OnMenu(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);

    ScopeCanvas *scopeCanvas;
    SpectrumCanvas *spectrumCanvas;
    WaterfallCanvas *waterfallCanvas;
    SpectrumCanvas *demodSpectrumCanvas;
    WaterfallCanvas *demodWaterfallCanvas;
    MeterCanvas *demodSignalMeter;
    TuningCanvas *demodTuner;
// event table

    DemodulatorInstance *activeDemodulator;

    std::vector<RtAudio::DeviceInfo> devices;
    std::map<int,RtAudio::DeviceInfo> inputDevices;
    std::map<int,RtAudio::DeviceInfo> outputDevices;
    std::map<int,wxMenuItem *> outputDeviceMenuItems;

    std::map<int,wxMenuItem *> demodMenuItems;

    wxDECLARE_EVENT_TABLE();
};
