// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "ui/imgui/BgfxCanvas.h"

#include <string>

class TuningCanvas : public BgfxCanvas {
public:
    enum ActiveState {
        TUNING_HOVER_NONE,
        TUNING_HOVER_FREQ,
        TUNING_HOVER_BW,
        TUNING_HOVER_PPM,
        TUNING_HOVER_CENTER
    };

    enum TuningDirection {
        TUNING_DIRECTION_DOWN,
        TUNING_DIRECTION_UP
    };

    TuningCanvas(wxWindow *parent);
    ~TuningCanvas() override;

    void setHelpTip(std::string tip);
    void setHalfBand(bool hb);
    void OnKeyDown(wxKeyEvent &event);
    void OnKeyUp(wxKeyEvent &event);
    ActiveState getHoverState();

    void frameUpdate() override;

private:
    struct ViewState {
        ActiveState hoverState = TUNING_HOVER_NONE;
        ActiveState downState = TUNING_HOVER_NONE;
        int hoverIndex = 0;
        int downIndex = 0;
        bool mouseDown = false;
        bool mouseRightDown = false;
        bool dragging = false;
        bool top = false;
        bool bottom = false;
        bool altDown = false;
        bool shiftDown = false;
        bool halfBand = false;
        bool firstPaintCheck = true;
        float dragAccum = 0.0f;
        float originMouseX = 0.0f;
        float originMouseY = 0.0f;
        float mouseMX = 0.0f;
        float mouseMY = 0.0f;
        int wheelAccumV = 0;
        int wheelAccumH = 0;
        long long paintedFreq = -1;
        long long paintedBw = -1;
        long long paintedCenter = -1;
        int paintedPPM = 0;
        bool paintedAlt = false;
        bool paintedShift = false;
        int currentPPM = 0;
        std::string helpTip;
        std::string lastTip;
    };

    void drawView(const BgfxEdges &edges, float mx, float my) override;
    void onEnterView() override;
    void onLeaveView() override;
    void StepTuner(ActiveState state, TuningDirection tuningDir, int digit, bool preventCarry, bool zeroOut);
    void updateHover(float mx01, float my01);

    ViewState view;
};
