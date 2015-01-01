#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "WaterfallContext.h"
#include "MouseTracker.h"
#include "SpectrumCanvas.h"

#include "fftw3.h"

class WaterfallCanvas: public wxGLCanvas {
public:
    enum DragState {
        WF_DRAG_NONE, WF_DRAG_BANDWIDTH_LEFT, WF_DRAG_BANDWIDTH_RIGHT, WF_DRAG_FREQUENCY, WF_DRAG_RANGE
    };

    WaterfallCanvas(wxWindow *parent, int *attribList = NULL);
    void Setup(int fft_size_in, int waterfall_lines_in);
    ~WaterfallCanvas();

    void setData(DemodulatorThreadIQData *input);
    int GetFrequencyAt(float x);

    void SetView(int center_freq_in, int bandwidth_in);
    void DisableView();

    void SetCenterFrequency(unsigned int center_freq_in);
    unsigned int GetCenterFrequency();

    void SetBandwidth(unsigned int bandwidth_in);
    unsigned int GetBandwidth();

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

    wxWindow *parent;
    SpectrumCanvas *spectrumCanvas;
    std::vector<float> spectrum_points;

    fftw_complex *in, *out;
    fftw_plan plan;

    float fft_ceil_ma, fft_ceil_maa;
    float fft_floor_ma, fft_floor_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    WaterfallContext *glContext;
    MouseTracker mTracker;

    int activeDemodulatorBandwidth;
    int activeDemodulatorFrequency;

    DragState dragState;
    DragState nextDragState;

    bool shiftDown;
    bool altDown;
    bool ctrlDown;

    int fft_size;
    int waterfall_lines;

    unsigned int center_freq;
    unsigned int bandwidth;

    bool isView;
    msresamp_crcf resampler;
    float resample_ratio;
    nco_crcf nco_shift;
    int shift_freq;

    int last_input_bandwidth;
    int last_bandwidth;

    std::vector<liquid_float_complex> shift_buffer;
    std::vector<liquid_float_complex> resampler_buffer;

    // event table
wxDECLARE_EVENT_TABLE();
};

