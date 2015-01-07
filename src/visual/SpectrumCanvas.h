#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "SpectrumContext.h"

#include "fftw3.h"
#include "MouseTracker.h"

class WaterfallCanvas;

class SpectrumCanvas: public InteractiveCanvas {
public:
    std::vector<float> spectrum_points;

    SpectrumCanvas(wxWindow *parent, int *attribList = NULL);
    void setup(int fft_size_in);
    ~SpectrumCanvas();

    void setData(DemodulatorThreadIQData *input);
    void attachWaterfallCanvas(WaterfallCanvas *canvas_in);

private:
    void OnPaint(wxPaintEvent& event);

    void OnIdle(wxIdleEvent &event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    fftwf_complex *in, *out;
    fftwf_plan plan;

    float fft_ceil_ma, fft_ceil_maa;
    float fft_floor_ma, fft_floor_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    SpectrumContext *glContext;
    WaterfallCanvas *waterfallCanvas;
    int fft_size;
// event table
wxDECLARE_EVENT_TABLE();
};

