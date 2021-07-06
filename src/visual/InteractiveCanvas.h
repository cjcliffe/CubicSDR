// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include "MouseTracker.h"
#include <string>
#include <vector>

class InteractiveCanvas: public wxGLCanvas {
public:
    InteractiveCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs);
    ~InteractiveCanvas() override;

    long long getFrequencyAt(float x);
    long long getFrequencyAt(float x, long long iqCenterFreq, long long iqBandwidth);
    
    virtual void setView(long long center_freq_in, long long bandwidth_in);
    virtual void disableView();
    bool getViewState() const;

    void setCenterFrequency(long long center_freq_in);
    long long getCenterFrequency() const;

    void setBandwidth(long long bandwidth_in);
    long long getBandwidth() const;

    MouseTracker *getMouseTracker();
    bool isMouseInView();
    bool isMouseDown();
    
    bool isAltDown() const;
    bool isCtrlDown() const;
    bool isShiftDown() const;

protected:
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseRightDown(wxMouseEvent& event);
    void OnMouseRightReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    void setStatusText(std::string statusText);
    void setStatusText(std::string statusText, int value);

    wxWindow *parent;
    MouseTracker mouseTracker;

    bool shiftDown;
    bool altDown;
    bool ctrlDown;

    long long centerFreq;
    long long bandwidth;
    long long lastBandwidth;

    bool isView;
};

