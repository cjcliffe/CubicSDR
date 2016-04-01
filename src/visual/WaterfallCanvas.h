#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "MouseTracker.h"
#include "SpectrumCanvas.h"
#include "WaterfallPanel.h"
#include "Timer.h"

class WaterfallCanvas: public InteractiveCanvas {
public:
    enum DragState {
        WF_DRAG_NONE, WF_DRAG_BANDWIDTH_LEFT, WF_DRAG_BANDWIDTH_RIGHT, WF_DRAG_FREQUENCY, WF_DRAG_RANGE
    };

    WaterfallCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs);
    void setup(unsigned int fft_size_in, int waterfall_lines_in);
    void setFFTSize(unsigned int fft_size_in);
    ~WaterfallCanvas();

    DragState getDragState();
    DragState getNextDragState();
    
    void attachSpectrumCanvas(SpectrumCanvas *canvas_in);
    void processInputQueue();
    SpectrumVisualDataQueue *getVisualDataQueue();

    void setLinesPerSecond(int lps);
    void setMinBandwidth(int min);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

private:
    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    void updateHoverState();
    
    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseRightDown(wxMouseEvent& event);
    void OnMouseRightReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    void updateCenterFrequency(long long freq);
    
    std::vector<float> spectrum_points;

    SpectrumCanvas *spectrumCanvas;
    PrimaryGLContext *glContext;
    WaterfallPanel waterfallPanel;

    DragState dragState;
    DragState nextDragState;

    unsigned int fft_size, new_fft_size;
    int waterfall_lines;
    int dragOfs;

    float mouseZoom, zoom;
    bool freqMoving;
    long double freqMove;
    float hoverAlpha;
    int linesPerSecond;
    float scaleMove;
    int dragBW;
    
    SpectrumVisualDataQueue visualDataQueue;
    Timer gTimer;
    double lpsIndex;
    bool preBuf;
    std::mutex tex_update;
    int minBandwidth;
    std::atomic_bool fft_size_changed;
    // event table
wxDECLARE_EVENT_TABLE();
};

