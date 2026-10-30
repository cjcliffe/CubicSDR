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
#include <chrono>
#include <cmath>

wxBEGIN_EVENT_TABLE(ModeSelectorCanvas, wxGLCanvas) EVT_PAINT(ModeSelectorCanvas::OnPaint)
EVT_MOTION(ModeSelectorCanvas::OnMouseMoved)
EVT_LEFT_DOWN(ModeSelectorCanvas::OnMouseDown)
EVT_LEFT_UP(ModeSelectorCanvas::OnMouseReleased)
EVT_LEAVE_WINDOW(ModeSelectorCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(ModeSelectorCanvas::OnMouseEnterWindow)
wxEND_EVENT_TABLE()

ModeSelectorCanvas::ModeSelectorCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs) :
InteractiveCanvas(parent, dispAttrs), numChoices(0), currentSelection(-1), toggleMode(false), inputChanged(false), padX(4.0), padY(4.0), highlightOverride(false), pulseMode(false), pulsePhase(0.0f), disabled(false) {

    glContext = new ModeSelectorContext(this, &wxGetApp().GetContext(this), wxGetApp().GetContextAttributes());
    
    highlightColor = RGBA4f(1.0,1.0,1.0,1.0);
}

ModeSelectorCanvas::~ModeSelectorCanvas() = default;

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
        if (yval == i && !highlightOverride && !disabled) {
            RGBA4f hc = ThemeMgr::mgr.currentTheme->buttonHighlight;
            glContext->DrawSelector(selections[i].label, i, numChoices, true, hc.r, hc.g, hc.b, 1.0, padX, padY);
        } else {
            RGBA4f hc = ThemeMgr::mgr.currentTheme->button;
            if (highlightOverride) {
                hc = highlightColor;
            }
            if (pulseMode && i == currentSelection) {
                // Breathe between the off-state look (outline plus
                // button-color text) and the on-state look (filled
                // button plus readable inverted text).
                float blend = 0.5f + 0.5f * sinf(pulsePhase);
                glContext->DrawSelector(selections[i].label, i, numChoices, false, hc.r, hc.g, hc.b, 1.0, padX, padY);
                glContext->DrawSelector(selections[i].label, i, numChoices, true, hc.r, hc.g, hc.b, blend, padX, padY);
            } else {
                float alpha = 1.0f;
                if (disabled) {
                    hc = ThemeMgr::mgr.currentTheme->button;
                    alpha = 0.3f;
                }
                glContext->DrawSelector(selections[i].label, i, numChoices, i == currentSelection, hc.r, hc.g, hc.b, alpha, padX, padY);
            }
        }
    }

    glContext->DrawEnd();

    SwapBuffers();
}

void ModeSelectorCanvas::frameUpdate() {
    if (pulseMode && currentSelection != -1) {
        // Wall-clock phase, one full breathe every two seconds: same rate
        // as the recording glow in the spectrum view. The modulo stays in
        // integer milliseconds so float precision cannot freeze the cycle.
        auto t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        pulsePhase = 6.2831853f * (float)(t % 2000) / 2000.0f;
        Refresh();
    } else if (mouseTracker.mouseInView()) {
        Refresh();
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

    if (disabled) {
        return;
    }

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

void ModeSelectorCanvas::setSelection(const std::string& label) {
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

void ModeSelectorCanvas::setToggleMode(bool toggleMode_in) {
    toggleMode = toggleMode_in;
}

bool ModeSelectorCanvas::modeChanged() const {
    return inputChanged;
}

void ModeSelectorCanvas::clearModeChanged() {
    inputChanged = false;
}

void ModeSelectorCanvas::setPadding(float padX_in, float padY_in) {
    padX = padX_in;
    padY = padY_in;
}

void ModeSelectorCanvas::setHighlightColor(const RGBA4f& hc) {
    highlightColor = hc;
    highlightOverride = true;
}

void ModeSelectorCanvas::setPulseMode(bool pulseMode_in) {
    pulseMode = pulseMode_in;
}

void ModeSelectorCanvas::setDisabled(bool disabled_in) {
    disabled = disabled_in;
    Refresh();
}

bool ModeSelectorCanvas::isDisabled() const {
    return disabled;
}
