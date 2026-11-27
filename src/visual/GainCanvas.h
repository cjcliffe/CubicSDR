// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "ui/imgui/BgfxCanvas.h"

#include <atomic>
#include <string>
#include <vector>

#include "SDRDeviceInfo.h"
#include "Timer.h"

class GainCanvas : public BgfxCanvas {
public:
    GainCanvas(wxWindow *parent);
    ~GainCanvas() override;

    void setHelpTip(std::string tip);
    void updateGainUI();

    void frameUpdate() override;

private:
    struct GainStrip {
        std::string name;
        float low = 0.0f;
        float high = 0.0f;
        float value = 0.0f;
        float highlight = 0.0f;
        bool changed = false;
        bool highlightVisible = false;
    };

    void drawView(const BgfxEdges &edges, float mx, float my) override;
    void onEnterView() override;
    void onLeaveView() override;
    bool updateGainValues();
    void setLevel(float mx01, float my01);

    std::vector<GainStrip> gainStrips;
    SDRRangeMap gains;

    float numGains;
    std::string helpTip;
    bool gainPressedHere;

    std::atomic_bool userGainAsChanged;
    Timer userGainAsChangedDelayTimer;
};
