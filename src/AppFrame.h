#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"


#include "ScopeCanvas.h"
#include "SpectrumCanvas.h"
#include "WaterfallCanvas.h"

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();
    void OnThread(wxCommandEvent& event);
    void OnEventInput(wxThreadEvent& event);


private:
    void OnClose(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);

    ScopeCanvas *scopeCanvas;
    SpectrumCanvas *spectrumCanvas;
    WaterfallCanvas *waterfallCanvas;


// event table
wxDECLARE_EVENT_TABLE();
};
