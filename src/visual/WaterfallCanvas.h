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
    void setup(int fft_size_in, int waterfall_lines_in);
    ~WaterfallCanvas();

    void setData(DemodulatorThreadIQData *input);

    DragState getDragState();
    DragState getNextDragState();

    void attachSpectrumCanvas(SpectrumCanvas *canvas_in);
    void attachWaterfallCanvas(WaterfallCanvas *canvas_in);

    bool isPolling();
    void setPolling(bool polling);

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseRightDown(wxMouseEvent& event);
    void OnMouseRightReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    std::vector<float> spectrum_points;

    SpectrumCanvas *spectrumCanvas;
    WaterfallCanvas *otherWaterfallCanvas;
    bool polling;

    fftwf_complex *in, *out, *fft_in_data, *fft_last_data;
    unsigned int last_data_size;
    fftwf_plan plan;

    float fft_ceil_ma, fft_ceil_maa;
    float fft_floor_ma, fft_floor_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    WaterfallContext *glContext;

    DragState dragState;
    DragState nextDragState;

    int fft_size;
    int waterfall_lines;
    int dragOfs;

    msresamp_crcf resampler;
    double resamplerRatio;
    nco_crcf freqShifter;
    long shiftFrequency;

    int lastInputBandwidth;
    float mouseZoom, zoom;
    float hoverAlpha;

    std::vector<liquid_float_complex> shiftBuffer;
    std::vector<liquid_float_complex> resampleBuffer;


    // event table
wxDECLARE_EVENT_TABLE();
};

