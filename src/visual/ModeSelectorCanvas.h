// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <string>
#include <vector>

#include "ui/imgui/BgfxCanvas.h"
#include "ColorTheme.h"

class ModeSelectorMode {
public:
    int value;
    std::string label;

    ModeSelectorMode(int value, std::string label) : value(value), label(label) {

    }
};

class ModeSelectorCanvas : public BgfxCanvas {
public:
    ModeSelectorCanvas(wxWindow *parent, const std::string &windowName = "ModeSelector");
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

    void frameUpdate() override;

private:
    void drawView(const BgfxEdges &edges, float mx, float my) override;
    void onEnterView() override;
    void onLeaveView() override;

    std::string viewName;
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

    bool pressed;
    float originX;
    float lastMx, lastMy;
};
