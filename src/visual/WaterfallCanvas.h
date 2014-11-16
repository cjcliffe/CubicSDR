#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "WaterfallContext.h"

#include "fftw3.h"

class WaterfallCanvas: public wxGLCanvas {
public:
    WaterfallCanvas(wxWindow *parent, int *attribList = NULL);
    ~WaterfallCanvas();

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

    WaterfallContext *glContext;
// event table
wxDECLARE_EVENT_TABLE();
};

