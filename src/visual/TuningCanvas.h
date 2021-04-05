// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "TuningContext.h"
#include "MouseTracker.h"

#include "Timer.h"

class TuningCanvas: public InteractiveCanvas {
public:
    enum ActiveState {
        TUNING_HOVER_NONE, TUNING_HOVER_FREQ, TUNING_HOVER_BW, TUNING_HOVER_PPM, TUNING_HOVER_CENTER
    };
    enum TuningDirection {
        TUNING_DIRECTION_DOWN, TUNING_DIRECTION_UP
    };
    TuningCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs);
    ~TuningCanvas() override;

    void setHelpTip(std::string tip);
    bool changed();
    
    void setHalfBand(bool hb);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    
    ActiveState getHoverState();
    
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

    void StepTuner(ActiveState state, TuningDirection tuningDir, int digit, bool preventCarry = false, bool zeroOut = false);

    TuningContext *glContext;

    std::string helpTip;
    float dragAccum;
    float uxDown;
    ActiveState hoverState;
    ActiveState downState;
    int hoverIndex;
    int downIndex;
    bool dragging;

    float freqDP;
    float freqW;

    float bwDP;
    float bwW;

    float centerDP;
    float centerW;

    bool top;
    bool bottom;

    int currentPPM;
    int lastPPM;

    long long freq, bw, center;
    bool halfBand;
    //
wxDECLARE_EVENT_TABLE();
};

