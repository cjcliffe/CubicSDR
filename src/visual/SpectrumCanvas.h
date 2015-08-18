#pragma once

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "PrimaryGLContext.h"
#include "MouseTracker.h"
#include "SpectrumVisualProcessor.h"
#include "SpectrumPanel.h"

class WaterfallCanvas;

class SpectrumCanvas: public InteractiveCanvas {
public:
    SpectrumCanvas(wxWindow *parent, int *attribList = NULL);
    ~SpectrumCanvas();

    void attachWaterfallCanvas(WaterfallCanvas *canvas_in);
    void moveCenterFrequency(long long freqChange);

    void setShowDb(bool showDb);
    bool getShowDb();
    
    SpectrumVisualDataQueue *getVisualDataQueue();
    
private:
    void OnPaint(wxPaintEvent& event);

    void OnIdle(wxIdleEvent &event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    PrimaryGLContext *glContext;
    WaterfallCanvas *waterfallCanvas;
    SpectrumPanel spectrumPanel;
    
    SpectrumVisualDataQueue visualDataQueue;

// event table
wxDECLARE_EVENT_TABLE();
};

