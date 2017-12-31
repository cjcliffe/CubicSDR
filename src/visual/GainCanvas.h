// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>
#include <atomic>

#include "InteractiveCanvas.h"
#include "MouseTracker.h"
#include "GLPanel.h"
#include "PrimaryGLContext.h"
#include "SDRDeviceInfo.h"
#include "Timer.h"
#include "MeterPanel.h"


class GainCanvas: public InteractiveCanvas {
public:
    GainCanvas(wxWindow *parent, std::vector<int> dispAttrs);
    ~GainCanvas();

    void setHelpTip(std::string tip);
    void updateGainUI();
	void setThemeColors();
    
private:

	// call this to refresh the gain values only, return true if refresh is needed
	bool updateGainValues();

    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    void SetLevel();

    void OnShow(wxShowEvent& event);
    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    PrimaryGLContext *glContext;
    std::string helpTip;
    std::vector<MeterPanel *> gainPanels;
    GLPanel bgPanel;
    SDRRangeMap gains;
    
    float spacing, barWidth, startPos, barHeight, numGains;
    int refreshCounter;
    wxSize clientSize;

	std::atomic_bool userGainAsChanged;
	Timer userGainAsChangedDelayTimer;
    //
wxDECLARE_EVENT_TABLE();
};

