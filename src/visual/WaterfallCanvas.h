#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "WaterfallContext.h"
#include "MouseTracker.h"
#include "SpectrumCanvas.h"


class WaterfallCanvas: public InteractiveCanvas {
public:
    enum DragState {
        WF_DRAG_NONE, WF_DRAG_BANDWIDTH_LEFT, WF_DRAG_BANDWIDTH_RIGHT, WF_DRAG_FREQUENCY, WF_DRAG_RANGE
    };

    WaterfallCanvas(wxWindow *parent, int *attribList = NULL);
    void setup(int fft_size_in, int waterfall_lines_in);
    ~WaterfallCanvas();

    DragState getDragState();
    DragState getNextDragState();
    
    void attachSpectrumCanvas(SpectrumCanvas *canvas_in);
    SpectrumVisualDataQueue *getVisualDataQueue();

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

    WaterfallContext *glContext;

    DragState dragState;
    DragState nextDragState;

    int fft_size;
    int waterfall_lines;
    int dragOfs;

    float mouseZoom, zoom;
    float hoverAlpha;

    SpectrumVisualDataQueue visualDataQueue;

    // event table
wxDECLARE_EVENT_TABLE();
};

