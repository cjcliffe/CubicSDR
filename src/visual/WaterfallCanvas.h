#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "WaterfallContext.h"
#include "MouseTracker.h"

#include "fftw3.h"
#include "Timer.h"

class WaterfallCanvas: public wxGLCanvas {
public:
    enum DragState { WF_DRAG_NONE, WF_DRAG_BANDWIDTH_LEFT, WF_DRAG_BANDWIDTH_RIGHT, WF_DRAG_FREQUENCY };

    WaterfallCanvas(wxWindow *parent, int *attribList = NULL);
    ~WaterfallCanvas();

    void setData(std::vector<signed char> *data);
    int GetFrequencyAt(float x);

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
    std::vector<float> spectrum_points;

    fftw_complex *in, *out;
    fftw_plan plan;

    float fft_ceil_ma, fft_ceil_maa;
    float fft_floor_ma, fft_floor_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    WaterfallContext *glContext;
    Timer timer;
    float frameTimer;
    MouseTracker mTracker;

    int activeDemodulatorBandwidth;
    int activeDemodulatorFrequency;

    DragState dragState;
    DragState nextDragState;

    bool shiftDown;

// event table
wxDECLARE_EVENT_TABLE();
};

