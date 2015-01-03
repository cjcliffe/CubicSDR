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

    fftw_complex *in, *out;
    fftw_plan plan;

    double fft_ceil_ma, fft_ceil_maa;
    double fft_floor_ma, fft_floor_maa;

    std::vector<double> fft_result;
    std::vector<double> fft_result_ma;
    std::vector<double> fft_result_maa;

    SpectrumContext *glContext;
    WaterfallCanvas *waterfallCanvas;
    int fft_size;
// event table
wxDECLARE_EVENT_TABLE();
};

