// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "MeterCanvas.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include "AppFrame.h"
#include <algorithm>

wxBEGIN_EVENT_TABLE(MeterCanvas, wxGLCanvas) EVT_PAINT(MeterCanvas::OnPaint)
EVT_IDLE(MeterCanvas::OnIdle)
EVT_MOTION(MeterCanvas::OnMouseMoved)
EVT_LEFT_DOWN(MeterCanvas::OnMouseDown)
EVT_LEFT_UP(MeterCanvas::OnMouseReleased)
EVT_MOUSEWHEEL(MeterCanvas::OnMouseWheelMoved)
EVT_RIGHT_DOWN(MeterCanvas::OnMouseRightDown)
EVT_RIGHT_UP(MeterCanvas::OnMouseRightReleased)
EVT_LEAVE_WINDOW(MeterCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(MeterCanvas::OnMouseEnterWindow)
wxEND_EVENT_TABLE()

MeterCanvas::MeterCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs) :
        InteractiveCanvas(parent, dispAttrs), level(0), level_min(0), level_max(1), inputValue(0), userInputValue(0), showUserInput(true) {

    glContext = new MeterContext(this, &wxGetApp().GetContext(this), wxGetApp().GetContextAttributes());
}

MeterCanvas::~MeterCanvas() {

}

void MeterCanvas::setLevel(float level_in) {
    level = level_in;
    Refresh();
}
float MeterCanvas::getLevel() {
    return level;
}

void MeterCanvas::setMax(float max_in) {
    level_max = max_in;
    Refresh();
}

void MeterCanvas::setMin(float min_in) {
    level_min = min_in;
    Refresh();
}

void MeterCanvas::setUserInputValue(float slider_in) {
    userInputValue = slider_in;
    Refresh();
}

void MeterCanvas::setInputValue(float slider_in) {
    userInputValue = inputValue = slider_in;
    Refresh();
}

bool MeterCanvas::inputChanged() {
    return (inputValue != userInputValue);
}

float MeterCanvas::getInputValue() {
    inputValue = userInputValue;
    return userInputValue;
}

void MeterCanvas::setShowUserInput(bool showUserInput) {
    this->showUserInput = showUserInput;
}

void MeterCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
 //   wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize() * GetContentScaleFactor();

    glContext->SetCurrent(*this);
    initGLExtensions();

    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();
    glContext->Draw(ThemeMgr::mgr.currentTheme->generalBackground.r, ThemeMgr::mgr.currentTheme->generalBackground.g, ThemeMgr::mgr.currentTheme->generalBackground.b, 0.5, 1.0);
    
    if (mouseTracker.mouseInView()) {
        glContext->Draw(0.4f, 0.4f, 0.4f, 0.5f, mouseTracker.getMouseY());
    }
    glContext->Draw(ThemeMgr::mgr.currentTheme->meterLevel.r, ThemeMgr::mgr.currentTheme->meterLevel.g, ThemeMgr::mgr.currentTheme->meterLevel.b, 0.5, (level-level_min) / (level_max-level_min));
    if (showUserInput) {
        glContext->Draw(ThemeMgr::mgr.currentTheme->meterValue.r, ThemeMgr::mgr.currentTheme->meterValue.g, ThemeMgr::mgr.currentTheme->meterValue.b, 0.5, (userInputValue-level_min) / (level_max-level_min));
    } 
    glContext->DrawEnd();

    SwapBuffers();
}

void MeterCanvas::OnIdle(wxIdleEvent &event) {
	if (mouseTracker.mouseInView()) {
	    Refresh();
	} else {
		event.Skip();
	}
}

void MeterCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);

    if (mouseTracker.mouseDown()) {
        userInputValue = mouseTracker.getMouseY() * (level_max-level_min) + level_min;
    } else {
        if (!helpTip.empty()) {
            setStatusText(helpTip);
        }
    }
}

void MeterCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
    userInputValue = mouseTracker.getMouseY() * (level_max-level_min) + level_min;
    mouseTracker.setHorizDragLock(true);
}

void MeterCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
    userInputValue = mouseTracker.getMouseY() * (level_max-level_min) + level_min;
}

void MeterCanvas::OnMouseRightDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseRightDown(event);
}

void MeterCanvas::OnMouseRightReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseRightReleased(event);
	if (showUserInput) {
		userInputValue = level - level * 0.02;
	}
}

void MeterCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
    float movement = 3.0 * (float)event.GetWheelRotation();

	float currentValue = 0;
	if (showUserInput) {
		currentValue = userInputValue;
	} else {
		currentValue = level;
	}

    currentValue = currentValue + ((movement / 100.0) * ((level_max - level_min) / 100.0));
  
	if (currentValue > level_max) {
		currentValue = level_max;
	}
	if (currentValue < level_min) {
		currentValue = level_min;
	}

	userInputValue = currentValue;
}

void MeterCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
    Refresh();
}

void MeterCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
#ifdef _WIN32
	if (wxGetApp().getAppFrame()->canFocus()) {
		this->SetFocus();
	}
#endif
}

void MeterCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}
