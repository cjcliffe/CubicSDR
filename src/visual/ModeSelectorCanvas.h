#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "ModeSelectorContext.h"
#include "MouseTracker.h"

#include "fftw3.h"
#include "Timer.h"

class ModeSelectorCanvas: public InteractiveCanvas {
public:
    ModeSelectorCanvas(wxWindow *parent, int *attribList = NULL);
    ~ModeSelectorCanvas();

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

    ModeSelectorContext *glContext;

    std::string helpTip;
    //
wxDECLARE_EVENT_TABLE();
};

