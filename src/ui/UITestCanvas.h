// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "UITestContext.h"
#include "MouseTracker.h"

#include "Timer.h"

class UITestCanvas: public InteractiveCanvas {
public:
    UITestCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs);
    ~UITestCanvas();
    
private:
    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);
    
    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);
    
    UITestContext *glContext;
    
    wxDECLARE_EVENT_TABLE();
};

