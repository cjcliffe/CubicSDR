#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "WaterfallContext.h"
#include "MouseTracker.h"
#include "SpectrumCanvas.h"

#include "fftw3.h"

class WaterfallCanvas: public InteractiveCanvas {
public:
    enum DragState {
        WF_DRAG_NONE, WF_DRAG_BANDWIDTH_LEFT, WF_DRAG_BANDWIDTH_RIGHT, WF_DRAG_FREQUENCY, WF_DRAG_RANGE
    };

    WaterfallCanvas(wxWindow *parent, int *attribList = NULL);
    void Setup(int fft_size_in, int waterfall_lines_in);
    ~WaterfallCanvas();

    void setData(DemodulatorThreadIQData *input);

    DragState getDragState();
    DragState getNextDragState();

    void attachSpectrumCanvas(SpectrumCanvas *canvas_in);

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void mouseEnterWindow(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);

    SpectrumCanvas *spectrumCanvas;
    std::vector<float> spectrum_points;

    fftw_complex *in, *out;
    fftw_plan plan;

    double fft_ceil_ma, fft_ceil_maa;
    double fft_floor_ma, fft_floor_maa;

    std::vector<double> fft_result;
    std::vector<double> fft_result_ma;
    std::vector<double> fft_result_maa;

    WaterfallContext *glContext;

    int activeDemodulatorBandwidth;
    int activeDemodulatorFrequency;

    DragState dragState;
    DragState nextDragState;

    int fft_size;
    int waterfall_lines;

    msresamp_crcf resampler;
    double resample_ratio;
    nco_crcf nco_shift;
    int shift_freq;

    int last_input_bandwidth;
    int zoom;

    std::vector<liquid_float_complex> shift_buffer;
    std::vector<liquid_float_complex> resampler_buffer;

    // event table
wxDECLARE_EVENT_TABLE();
};

