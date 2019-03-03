// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>
#include <memory>
#include "InteractiveCanvas.h"
#include "MouseTracker.h"
#include "SpectrumCanvas.h"
#include "WaterfallPanel.h"
#include "Timer.h"
#include "SpinMutex.h"

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
    SpectrumVisualDataQueuePtr getVisualDataQueue();

    void setLinesPerSecond(int lps);
    void setMinBandwidth(int min);

    //This is public because it is indeed forwarded from
    //AppFrame::OnGlobalKeyDown, because global key handler intercepts 
    //calls in all windows.
    void OnKeyDown(wxKeyEvent& event);

    //This is public because it is indeed forwarded from
    //AppFrame::OnGlobalKeyUp, because global key handler intercepts 
    //calls in all windows.
    void OnKeyUp(wxKeyEvent& event);

    //public because called by SpectrumCanvas.
    void OnMouseWheelMoved(wxMouseEvent& event);
    
    
    
private:
    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    void updateHoverState();
    
    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseRightDown(wxMouseEvent& event);
    void OnMouseRightReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    void updateCenterFrequency(long long freq);
    
    std::vector<float> spectrum_points;

    SpectrumCanvas *spectrumCanvas = nullptr;
    PrimaryGLContext *glContext = nullptr;
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
    
    SpectrumVisualDataQueuePtr visualDataQueue = std::make_shared<SpectrumVisualDataQueue>();

    Timer gTimer;
    double lpsIndex;
    bool preBuf;
    SpinMutex tex_update;
    int minBandwidth;
    std::atomic_bool fft_size_changed;
    // event table
wxDECLARE_EVENT_TABLE();
};

