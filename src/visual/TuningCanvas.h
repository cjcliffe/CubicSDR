#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "TuningContext.h"
#include "MouseTracker.h"

#include "fftw3.h"
#include "Timer.h"

class TuningCanvas: public InteractiveCanvas {
public:
    enum HoverState {
        TUNING_HOVER_NONE, TUNING_HOVER_FREQ, TUNING_HOVER_BW, TUNING_HOVER_CENTER
    };
    TuningCanvas(wxWindow *parent, int *attribList = NULL);
    ~TuningCanvas();

    void setHelpTip(std::string tip);

private:
    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    TuningContext *glContext;

    std::string helpTip;
    float dragAccum;
    float uxDown;
    HoverState hoverState;
    int hoverIndex;

    float freqDP;
    float freqW;

    float bwDP;
    float bwW;

    float centerDP;
    float centerW;
    //
wxDECLARE_EVENT_TABLE();
};

