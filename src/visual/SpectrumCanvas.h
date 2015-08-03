#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "SpectrumContext.h"

#include "fftw3.h"
#include "MouseTracker.h"
#include "SpectrumVisualProcessor.h"

class WaterfallCanvas;

class SpectrumCanvas: public InteractiveCanvas {
public:
    std::vector<float> spectrum_points;

    SpectrumCanvas(wxWindow *parent, int *attribList = NULL);
    ~SpectrumCanvas();

    void attachWaterfallCanvas(WaterfallCanvas *canvas_in);
    void moveCenterFrequency(long long freqChange);

    SpectrumContext* getSpectrumContext();
    SpectrumVisualDataQueue *getVisualDataQueue();
    
private:
    void OnPaint(wxPaintEvent& event);

    void OnIdle(wxIdleEvent &event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    SpectrumContext *glContext;
    WaterfallCanvas *waterfallCanvas;
    
    SpectrumVisualDataQueue visualDataQueue;

// event table
wxDECLARE_EVENT_TABLE();
};

