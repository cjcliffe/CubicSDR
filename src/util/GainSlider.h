// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cmath>
#include <string>

namespace GainSlider {

struct GainStrip {
    float spacing = 0.0f;
    float barWidth = 0.0f;
    float startPos = 0.0f;
    float barHeight = 1.0f;
};

struct LabelLayout {
    double bgH = 0.0;
    double valY = 0.0;
    double valH = 0.0;
    double labY = 0.0;
    double labH = 0.0;
};

inline float hitValue(float hitY01, float low, float high) {
    float hitLevel = ((hitY01 + 1.0) * 0.5);
    if (hitLevel < 0.0f) {
        hitLevel = 0.0f;
    }
    if (hitLevel > 1.0f) {
        hitLevel = 1.0f;
    }
    return low + (hitLevel * (high - low));
}

inline float clampGain(float value, float low, float high) {
    if (value > high) {
        value = high;
    }
    if (value < low) {
        value = low;
    }
    return value;
}

inline std::string gainLabelText(float value) {
    return std::to_string(int(value));
}

inline float levelNorm(float value, float low, float high) {
    return (value - low) / (high - low);
}

inline LabelLayout meterLabelLayout(double viewHeightPx, double hScale, double labelHeightPx) {
    double viewHeight = viewHeightPx * hScale;
    double labelPad = 8.0;
    double pScale = (1.0 / viewHeight);
    LabelLayout out;
    out.bgH = 1.0 - pScale * (labelHeightPx + labelPad * 2.0);
    out.valY = (pScale * (labelHeightPx / 2.0 + labelPad)) - 1.0;
    out.valH = pScale * labelHeightPx;
    out.labY = 1.0 - (pScale * (labelHeightPx / 2.0 + labelPad));
    out.labH = pScale * labelHeightPx;
    return out;
}

inline GainStrip gainStripLayout(int numGains) {
    GainStrip out;
    out.spacing = 2.0 / numGains;
    out.barWidth = (1.0 / numGains) * 0.7;
    out.startPos = out.spacing / 2.0;
    out.barHeight = 1.0f;
    return out;
}

inline float gainBarMidX(const GainStrip &strip, int index) {
    return -1.0 + strip.startPos + strip.spacing * (float)index;
}

inline float wheelGainStep(float currentValue, int wheelRotation, float low, float high) {
    float movement = 3.0 * (float)wheelRotation;
    return currentValue + ((movement / 100.0) * ((high - low) / 100.0));
}

inline float roundGain(double deviceGain) {
    return (float)std::round(deviceGain);
}

inline bool gainDisplayDiffers(float roundedGain, float panelValue) {
    return (int)roundedGain != (int)panelValue;
}

}
