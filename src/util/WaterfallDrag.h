// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cmath>

namespace WaterfallDrag {

inline int bandwidthDragStep(float deltaMouseX, long long viewBandwidthHz, bool leftSide, int baseBwHz, int minBwHz, int maxBwHz) {
    int bwDiff = (int)(deltaMouseX * (float)viewBandwidthHz) * 2;
    if (leftSide) {
        bwDiff = -bwDiff;
    }
    int currentBW = baseBwHz + bwDiff;
    if (currentBW > maxBwHz) {
        currentBW = maxBwHz;
    }
    if (currentBW < minBwHz) {
        currentBW = minBwHz;
    }
    return currentBW;
}

inline long long snapDelta(long long diff, int snap) {
    if (snap > 1) {
        return (long long)(roundf((float)diff / (float)snap) * snap);
    }
    return diff;
}

inline long long dragOffset(float mouseX01, long long viewBandwidthHz, long long viewCenterHz, long long demodFreqHz) {
    return (long long)(mouseX01 * (float)viewBandwidthHz) + viewCenterHz - (viewBandwidthHz / 2) - demodFreqHz;
}

inline float wheelZoom(int wheelRotation, int wheelDelta) {
    int delta = wheelDelta ? wheelDelta : 120;
    float notches = (float)wheelRotation / (float)delta;
    float movement = notches * 40.0f;
    return 1.0f - movement / 1000.0f;
}

inline float glideZoom(int startYPx, int mouseYPx, int viewHeightPx) {
    float offset = (float)(startYPx - mouseYPx) / (float)viewHeightPx;
    return 1.0f - offset * (float)fabs(offset) * 0.2f;
}

}
