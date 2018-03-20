// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <vector>
#include <queue>
#include <memory>

#include "InteractiveCanvas.h"
#include "PrimaryGLContext.h"
#include "MouseTracker.h"
#include "SpectrumVisualProcessor.h"
#include "SpectrumPanel.h"

class WaterfallCanvas;

class SpectrumCanvas: public InteractiveCanvas {
public:
    SpectrumCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs);
    ~SpectrumCanvas();

    //This is public because it is indeed forwarded from
    //AppFrame::OnGlobalKeyDown, because global key handler intercepts 
    //calls in all windows.
    void OnKeyDown(wxKeyEvent& event);

    //This is public because it is indeed forwarded from
    //AppFrame::OnGlobalKeyUp, because global key handler intercepts 
    //calls in all windows.
    void OnKeyUp(wxKeyEvent& event);

    void attachWaterfallCanvas(WaterfallCanvas *canvas_in);
    void moveCenterFrequency(long long freqChange);

    void setShowDb(bool showDb);
    bool getShowDb();
    
    void setUseDBOfs(bool showDb);
    bool getUseDBOfs();
    
    void setView(long long center_freq_in, int bandwidth_in);
    void disableView();

    void setScaleFactorEnabled(bool en);
    void setFFTSize(int fftSize);
    
    SpectrumVisualDataQueuePtr getVisualDataQueue();
    
    // called by Waterfall to forward the update of the vertical scale.
    void updateScaleFactorFromYMove(float yDeltaMouseMove);
    
private:
    void OnPaint(wxPaintEvent& event);

    void OnIdle(wxIdleEvent &event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);
    void OnMouseRightDown(wxMouseEvent& event);
    void OnMouseRightReleased(wxMouseEvent& event);
   
    void updateScaleFactor(float factor);
    
    PrimaryGLContext *glContext;
    WaterfallCanvas *waterfallCanvas;
    SpectrumPanel spectrumPanel;
    float scaleFactor;
    int bwChange;
    bool resetScaleFactor, scaleFactorEnabled;
    
    SpectrumVisualDataQueuePtr  visualDataQueue = std::make_shared<SpectrumVisualDataQueue>();

// event table
wxDECLARE_EVENT_TABLE();
};

