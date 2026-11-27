// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "ui/imgui/BgfxCanvas.h"

#include <string>

class MeterCanvas : public BgfxCanvas {
public:
    MeterCanvas(wxWindow *parent, const char *viewName);
    ~MeterCanvas() override;

    void setLevel(float level_in);
    float getLevel() const;

    void setMax(float max_in);
    void setMin(float min_in);

    void setUserInputValue(float slider_in);
    void setInputValue(float slider_in);
    void setDefaultValue(float default_in);
    bool inputChanged() const;
    float getInputValue();
    void setShowUserInput(bool showUserInput_in);
    void setSnapOffset(float snapOffset_in);

    void setHelpTip(std::string tip);

private:
    void drawView(const BgfxEdges &edges, float mx, float my) override;
    void onEnterView() override;
    void onLeaveView() override;
    void resetToDefault();
    void meterWheelStep(int rotation);

    std::string windowName;
    float level = 0.0f;
    float levelMin = 0.0f;
    float levelMax = 1.0f;
    float inputValue = 0.0f;
    float userInputValue = 0.0f;
    float defaultValue = 0.0f;
    float snapOffset = 6.0f;
    bool hasDefault = false;
    bool suppressNextRelease = false;
    bool showUserInput = true;
    bool pressedHere = false;
    bool rightPressedHere = false;
    std::string helpTip;
};
