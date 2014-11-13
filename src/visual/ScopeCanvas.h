#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "ScopeContext.h"

#include "fftw3.h"
#include "Demodulator.h"

class ScopeCanvas: public wxGLCanvas {
public:
    ScopeCanvas(wxWindow *parent, int *attribList = NULL);
    ~ScopeCanvas();

    void setData(std::vector<signed char> *data);

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

    wxWindow *parent;
    std::vector<float> spectrum_points;

    fftw_complex *in, *out[2];
    fftw_plan plan[2];

    float fft_ceil_ma, fft_ceil_maa;
    float fft_floor_ma, fft_floor_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    Demodulator test_demod;
    ScopeContext *glContext;wxDECLARE_EVENT_TABLE();
};

