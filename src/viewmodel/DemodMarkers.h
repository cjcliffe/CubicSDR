// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Pure geometry for demodulator markers: NDC positions, sideband edges, labels.
// Float formulas match the former inline code exactly; degenerate inputs yield
// empty geometry instead.

#pragma once

#include <cmath>
#include <string>

#include "visual/ColorTheme.h"

namespace DemodMarkers {

constexpr double PI = 3.141592653589793;

enum class Sideband { Both, UpperOnly, LowerOnly };
enum class LabelAlign { Left, Right, Center };

struct MarkerQuad {
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
};

struct MarkerInput {
    long long freqHz = 0;
    long long bandwidthHz = 0;
    Sideband sideband = Sideband::Both;
    long long viewCenterHz = 0;
    long long viewSpanHz = 0;
    int viewWidthPx = 0;
    int viewHeightPx = 0;
    bool stackLabels = false;
    bool deltaLocked = false;
    bool muted = false;
    bool solo = false;
    bool soloMode = false;
    bool recording = false;
    long long timeMs = 0;
};

struct MarkerGeometry {
    float uxPos = 0.0f;
    float ofs = 0.0f;
    float ofsLeft = 0.0f;
    float ofsRight = 0.0f;
    float labelHeight = 0.0f;
    float hPos = 0.0f;
    MarkerQuad label;
    MarkerQuad band;
    bool narrowBand = false;
    LabelAlign align = LabelAlign::Center;
    std::string prefix;
    RGBA4f bg = RGBA4f(0.0f, 0.0f, 0.0f, 0.35f);
    float shadowDx = 0.0f;
    float shadowDy = 0.0f;
    std::string freqText;
};

inline float markerX(long long freqHz, long long viewCenterHz, long long viewSpanHz) {
    if (viewSpanHz <= 0) {
        return 0.0f;
    }
    float ux01 = (float)(freqHz - (viewCenterHz - viewSpanHz / 2)) / (float)viewSpanHz;
    return (ux01 - 0.5f) * 2.0f;
}

inline Sideband sidebandForType(const std::string &demodType) {
    if (demodType == "USB") {
        return Sideband::UpperOnly;
    }
    if (demodType == "LSB") {
        return Sideband::LowerOnly;
    }
    return Sideband::Both;
}

inline std::string demodTypeLabel(const std::string &type, bool locked) {
    return locked ? type + " Lock" : type;
}

inline std::string freqLabelText(long long freqHz) {
    return std::to_string((double)freqHz / 1000000.0);
}

inline MarkerGeometry markerGeometry(const MarkerInput &in) {
    MarkerGeometry geo;
    if (in.viewSpanHz <= 0 || in.viewWidthPx <= 0 || in.viewHeightPx <= 0) {
        return geo;
    }

    geo.uxPos = markerX(in.freqHz, in.viewCenterHz, in.viewSpanHz);
    geo.ofs = (float)in.bandwidthHz / (float)in.viewSpanHz;
    geo.ofsLeft = (in.sideband != Sideband::UpperOnly) ? geo.ofs : 0.0f;
    geo.ofsRight = (in.sideband != Sideband::LowerOnly) ? geo.ofs : 0.0f;

    geo.labelHeight = (float)(20.0 / (double)in.viewHeightPx);
    geo.hPos = (float)(-1.0 + (in.stackLabels ? (geo.labelHeight * 3.0) : geo.labelHeight));

    geo.label = {geo.uxPos - geo.ofsLeft, -1.0f, geo.uxPos + geo.ofsRight, geo.hPos + geo.labelHeight};
    geo.band = {geo.uxPos - geo.ofsLeft, -1.0f, geo.uxPos + geo.ofsRight, 1.0f};
    geo.narrowBand = (double)geo.ofs * 2.0 < 16.0 / (double)in.viewWidthPx;

    if (in.sideband == Sideband::UpperOnly) {
        geo.align = LabelAlign::Left;
    } else if (in.sideband == Sideband::LowerOnly) {
        geo.align = LabelAlign::Right;
    }

    std::string flags;
    if (in.deltaLocked) {
        flags.append("V");
    }
    if (in.recording) {
        flags.append("R");
    }
    if (in.muted) {
        flags.append("M");
    } else if (in.solo) {
        flags.append("S");
    }
    if (!flags.empty()) {
        geo.prefix = "[" + flags + "] ";
    }

    if (in.solo) {
        geo.bg.r = geo.bg.g = 0.8f;
    } else if (in.muted) {
        geo.bg.r = 0.8f;
    } else if (in.soloMode) {
        geo.bg.r = 0.2f;
    }
    if (in.recording) {
        geo.bg.g = sinf((float)(2.0f * PI * (double)(in.timeMs % 2000) / 2000.0)) * 0.25f + 0.75f;
    }

    geo.shadowDx = (float)(4.0 / (double)in.viewWidthPx);
    geo.shadowDy = (float)(2.0 / (double)in.viewHeightPx);
    geo.freqText = freqLabelText(in.freqHz);
    return geo;
}

}
