#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "ScopeContext.h"

#include "fftw3.h"

class ScopeCanvas: public wxGLCanvas {
public:
    ScopeCanvas(wxWindow *parent, int *attribList = NULL);
    ~ScopeCanvas();

    void setWaveformPoints(std::vector<float> &waveform_points_in);
private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

    wxWindow *parent;
    std::vector<float> waveform_points;

    ScopeContext *glContext;
// event table
wxDECLARE_EVENT_TABLE();
};

