#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "ScopeContext.h"
#include "ScopeVisualProcessor.h"
#include "ScopePanel.h"
#include "fftw3.h"

class ScopeCanvas: public wxGLCanvas {
public:
    ScopeCanvas(wxWindow *parent, int *attribList = NULL);
    ~ScopeCanvas();

    void setStereo(bool state);
    void setDeviceName(std::string device_name);
    void setPPMMode(bool ppmMode);
    bool getPPMMode();

    ScopeRenderDataQueue *getInputQueue();
    
private:
    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    ScopeRenderDataQueue inputData;
    ScopePanel scopePanel;
    ScopeContext *glContext;
    std::string deviceName;
    bool stereo;
    bool ppmMode;
// event table
wxDECLARE_EVENT_TABLE();
};

