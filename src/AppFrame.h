#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"

#include "ScopeCanvas.h"
#include "SpectrumCanvas.h"
#include "WaterfallCanvas.h"
#include "MeterCanvas.h"

#include <map>

#define wxID_RT_AUDIO_DEVICE 10000

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();
    void OnThread(wxCommandEvent& event);
    void OnEventInput(wxThreadEvent& event);

private:
    void OnMenu(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);

    ScopeCanvas *scopeCanvas;
    SpectrumCanvas *spectrumCanvas;
    WaterfallCanvas *waterfallCanvas;
    SpectrumCanvas *demodSpectrumCanvas;
    WaterfallCanvas *demodWaterfallCanvas;
    MeterCanvas *demodSignalMeter;
// event table

    DemodulatorInstance *activeDemodulator;

    std::map<int,RtAudio::DeviceInfo> input_devices;
    std::map<int,RtAudio::DeviceInfo> output_devices;
    std::map<int,wxMenuItem *> output_device_menuitems;

    wxDECLARE_EVENT_TABLE();
};
