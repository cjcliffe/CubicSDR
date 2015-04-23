#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "ScopeContext.h"
#include "fftw3.h"

class ScopeCanvas: public wxGLCanvas {
public:
    std::vector<float> waveform_points;

    ScopeCanvas(wxWindow *parent, int *attribList = NULL);
    ~ScopeCanvas();

    void setWaveformPoints(std::vector<float> &waveform_points_in);
    void setStereo(bool state);
    void setDeviceName(std::string device_name);
    void setPPMMode(bool ppmMode);
    bool getPPMMode();

private:
    void OnPaint(wxPaintEvent& event);

    void OnIdle(wxIdleEvent &event);

    wxWindow *parent;

    ScopeContext *glContext;
    std::string deviceName;
    bool stereo;
    bool ppmMode;
// event table
wxDECLARE_EVENT_TABLE();
};

