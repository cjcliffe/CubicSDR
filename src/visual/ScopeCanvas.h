#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "ScopeContext.h"
#include "ScopeVisualProcessor.h"
#include "ScopePanel.h"
#include "fftw3.h"
#include "InteractiveCanvas.h"

class ScopeCanvas: public InteractiveCanvas {
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
    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    ScopeRenderDataQueue inputData;
    ScopePanel scopePanel;
    GLPanel bgPanel;
    ScopeContext *glContext;
    std::string deviceName;
    bool stereo;
    bool ppmMode;
    float panelSpacing;
    float ctr;
    float ctrTarget;
    float dragAccel;
// event table
wxDECLARE_EVENT_TABLE();
};

