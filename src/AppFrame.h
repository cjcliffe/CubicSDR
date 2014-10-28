#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"

// Define a new frame type
class AppFrame : public wxFrame
{
public:
    AppFrame();

private:
    void OnClose(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};
