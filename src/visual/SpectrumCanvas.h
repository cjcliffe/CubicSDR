#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "SpectrumContext.h"

#include "fftw3.h"

class SpectrumCanvas: public wxGLCanvas {
public:
    SpectrumCanvas(wxWindow *parent, int *attribList = NULL);
    ~SpectrumCanvas();

    void setData(std::vector<signed char> *data);
private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

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
// event table
wxDECLARE_EVENT_TABLE();
};

