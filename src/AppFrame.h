#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    void OnEventInput(wxThreadEvent& event);

private:
    void OnClose(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);

    TestGLCanvas *canvas;

wxDECLARE_EVENT_TABLE();
};
