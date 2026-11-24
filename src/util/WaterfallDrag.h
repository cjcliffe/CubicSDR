// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cmath>

#include "DemodMarkers.h"

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

struct RangeDrag {
    long long freqHz = 0;
    unsigned int bandwidthHz = 0;
};

inline long long clickPlaceFreq(float pos01, long long viewCenterHz, long long viewBandwidthHz, bool haveDemod, long long demodFreqHz,
        int snap) {
    long long freqTarget = viewCenterHz - (long long)(0.5 * (float)viewBandwidthHz) + (long long)((float)pos01 * (float)viewBandwidthHz);
    long long demodFreq = haveDemod ? demodFreqHz : freqTarget;
    long long bwDiff = freqTarget - demodFreq;
    long long freq = demodFreq;
    if (snap > 1) {
        if (haveDemod) {
            bwDiff = roundf((double)bwDiff / (double)snap) * snap;
            freq += bwDiff;
        } else {
            freq = roundl((long double)freq / (double)snap) * snap;
        }
    } else {
        freq += bwDiff;
    }
    return freq;
}

inline RangeDrag rangeDrag(float originX01, float mouseX01, float deltaX01, DemodMarkers::Sideband sideband, long long viewCenterHz,
        long long viewBandwidthHz, int snap, int minBwHz) {
    RangeDrag range;
    float width = deltaX01;
    float pos;
    if (sideband == DemodMarkers::Sideband::LowerOnly || sideband == DemodMarkers::Sideband::UpperOnly) {
        float pos1 = originX01;
        float pos2 = mouseX01;
        if (pos2 < pos1) {
            float tmp = pos1;
            pos1 = pos2;
            pos2 = tmp;
        }
        pos = (sideband == DemodMarkers::Sideband::LowerOnly) ? pos2 : pos1;
        width *= 2;
    } else {
        pos = originX01 + width / 2.0;
    }
    long long freq = viewCenterHz - (long long)(0.5 * (float)viewBandwidthHz) + (long long)((float)pos * (float)viewBandwidthHz);
    unsigned int bw = (unsigned int)(fabs(width) * (float)viewBandwidthHz);
    if (bw < minBwHz) {
        bw = minBwHz;
    }
    if (snap > 1) {
        freq = roundl((long double)freq / (double)snap) * snap;
    }
    range.freqHz = freq;
    range.bandwidthHz = bw;
    return range;
}
}
