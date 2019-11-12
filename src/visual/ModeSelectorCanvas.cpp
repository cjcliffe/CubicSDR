// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModeSelectorCanvas.h"

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

wxBEGIN_EVENT_TABLE(ModeSelectorCanvas, wxGLCanvas) EVT_PAINT(ModeSelectorCanvas::OnPaint)
EVT_IDLE(ModeSelectorCanvas::OnIdle)
EVT_MOTION(ModeSelectorCanvas::OnMouseMoved)
EVT_LEFT_DOWN(ModeSelectorCanvas::OnMouseDown)
EVT_LEFT_UP(ModeSelectorCanvas::OnMouseReleased)
EVT_LEAVE_WINDOW(ModeSelectorCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(ModeSelectorCanvas::OnMouseEnterWindow)
wxEND_EVENT_TABLE()

ModeSelectorCanvas::ModeSelectorCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs) :
InteractiveCanvas(parent, dispAttrs), numChoices(0), currentSelection(-1), toggleMode(false), inputChanged(false), padX(4.0), padY(4.0), highlightOverride(false) {

    glContext = new ModeSelectorContext(this, &wxGetApp().GetContext(this), wxGetApp().GetContextAttributes());
    
    highlightColor = RGBA4f(1.0,1.0,1.0,1.0);
}

ModeSelectorCanvas::~ModeSelectorCanvas() {

}

int ModeSelectorCanvas::getHoveredSelection() {
    if (!mouseTracker.mouseInView()) {
        return -1;
    }

    float ypos = 1.0 - (mouseTracker.getMouseY() * 2.0);
    float yval = (int) (((ypos + 1.0) / 2.0) * (float) numChoices);

    return yval;
}

void ModeSelectorCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
   // wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize() * GetContentScaleFactor();

    glContext->SetCurrent(*this);
    initGLExtensions();

    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();

    int yval = getHoveredSelection();

    for (int i = 0; i < numChoices; i++) {
        if (yval == i && !highlightOverride) {
            RGBA4f hc = ThemeMgr::mgr.currentTheme->buttonHighlight;
            glContext->DrawSelector(selections[i].label, i, numChoices, true, hc.r, hc.g, hc.b, 1.0, padX, padY);
        } else {
            RGBA4f hc = ThemeMgr::mgr.currentTheme->button;
            if (highlightOverride) {
                hc = highlightColor;
            }
            glContext->DrawSelector(selections[i].label, i, numChoices, i == currentSelection, hc.r, hc.g, hc.b, 1.0, padX, padY);
        }
    }

    glContext->DrawEnd();

    SwapBuffers();
}

void ModeSelectorCanvas::OnIdle(wxIdleEvent &event) {
	if (mouseTracker.mouseInView()) {
		Refresh();
	} else {
		event.Skip();
	}
}

void ModeSelectorCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);
}

void ModeSelectorCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
    mouseTracker.setHorizDragLock(true);
    mouseTracker.setVertDragLock(true);
}

void ModeSelectorCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
}

void ModeSelectorCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
    mouseTracker.setHorizDragLock(false);
    mouseTracker.setVertDragLock(false);

    const wxSize ClientSize = GetClientSize();

    int selectedButton = currentSelection;
    if (mouseTracker.getOriginDeltaMouseX() < 2.0 / ClientSize.y) {
        selectedButton = getHoveredSelection();
    }

    if (toggleMode && (currentSelection == selectedButton)) {
        selectedButton = -1;
    }
    
    if (currentSelection != selectedButton) {
        inputChanged = true;
    }
    
    currentSelection = selectedButton;
    
    SetCursor (wxCURSOR_HAND);
    Refresh();
}

void ModeSelectorCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor (wxCURSOR_CROSS);
    Refresh();
}

void ModeSelectorCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
    SetCursor (wxCURSOR_HAND);
    if (!helpTip.empty()) {
        setStatusText(helpTip);
    }
    Refresh();
}

void ModeSelectorCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

void ModeSelectorCanvas::setNumChoices(int numChoices_in) {
    numChoices = numChoices_in;
    Refresh();
}

void ModeSelectorCanvas::addChoice(int value, std::string label) {
    selections.push_back(ModeSelectorMode(value, label));
    numChoices = selections.size();
}

void ModeSelectorCanvas::addChoice(std::string label) {
    selections.push_back(ModeSelectorMode(selections.size()+1, label));
    numChoices = selections.size();
}

void ModeSelectorCanvas::setSelection(std::string label) {
    for (int i = 0; i < numChoices; i++) {
        if (selections[i].label == label) {
            currentSelection = i;
            Refresh();
            return;
        }
    }
    currentSelection = -1;
    Refresh();
}

std::string ModeSelectorCanvas::getSelectionLabel() {
    if (currentSelection == -1) {
        return "";
    }
    return selections[currentSelection].label;
}

void ModeSelectorCanvas::setSelection(int value) {
    for (int i = 0; i < numChoices; i++) {
        if (selections[i].value == value) {
            currentSelection = i;
            Refresh();
            return;
        }
    }
    currentSelection = -1;
    Refresh();
}

int ModeSelectorCanvas::getSelection() {
    if (currentSelection == -1) {
        return -1;
    }
    return selections[currentSelection].value;
}

void ModeSelectorCanvas::setToggleMode(bool toggleMode) {
    this->toggleMode = toggleMode;
}

bool ModeSelectorCanvas::modeChanged() {
    return inputChanged;
}

void ModeSelectorCanvas::clearModeChanged() {
    inputChanged = false;
}

void ModeSelectorCanvas::setPadding(float padX, float padY) {
    this->padX = padX;
    this->padY = padY;
}

void ModeSelectorCanvas::setHighlightColor(RGBA4f hc) {
    this->highlightColor = hc;
    this->highlightOverride = true;
}
