#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "SpectrumContext.h"

#include "fftw3.h"
#include "Timer.h"
#include "MouseTracker.h"

class SpectrumCanvas: public wxGLCanvas {
public:
    SpectrumCanvas(wxWindow *parent, int *attribList = NULL);
    void Setup(int fft_size_in);
    ~SpectrumCanvas();

    void setData(DemodulatorThreadIQData *input);
private:
    void OnPaint(wxPaintEvent& event);

    void OnIdle(wxIdleEvent &event);

    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);

//    void rightClick(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);

    wxWindow *parent;
    std::vector<float> spectrum_points;

    fftw_complex *in, *out;
    fftw_plan plan;

    float fft_ceil_ma, fft_ceil_maa;
    float fft_floor_ma, fft_floor_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    SpectrumContext *glContext;
    int fft_size;

    MouseTracker mTracker;
// event table
wxDECLARE_EVENT_TABLE();
};

