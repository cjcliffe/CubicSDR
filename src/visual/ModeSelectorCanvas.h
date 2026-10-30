// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "ModeSelectorContext.h"
#include "MouseTracker.h"

#include "Timer.h"

class ModeSelectorMode {
public:
    int value;
    std::string label;

    ModeSelectorMode(int value, std::string label) : value(value), label(label) {

    }
};

class ModeSelectorCanvas: public InteractiveCanvas {
public:
    ModeSelectorCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs);
    ~ModeSelectorCanvas() override;

    int getHoveredSelection();
    void setHelpTip(std::string tip);

    void addChoice(int value, std::string label);
    void addChoice(std::string label);
    void setSelection(const std::string& label);
    std::string getSelectionLabel();
    void setSelection(int value);
    int getSelection();

    void setToggleMode(bool toggleMode_in);

    bool modeChanged() const;
    void clearModeChanged();
    
    void setPadding(float padX_in, float padY_in);
    void setHighlightColor(const RGBA4f& hc);
    void setPulseMode(bool pulseMode_in);
    void setDisabled(bool disabled_in);
    bool isDisabled() const;
    
private:
    void setNumChoices(int numChoices_in);

    void OnPaint(wxPaintEvent& event);
    void frameUpdate() override;

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    ModeSelectorContext *glContext;

    std::string helpTip;
    int numChoices;
    int currentSelection;
    bool toggleMode;
    bool inputChanged;
    std::vector<ModeSelectorMode> selections;
    float padX, padY;
    RGBA4f highlightColor;
    bool highlightOverride;
    bool pulseMode;
    float pulsePhase;
    bool disabled;
    //
wxDECLARE_EVENT_TABLE();
};

