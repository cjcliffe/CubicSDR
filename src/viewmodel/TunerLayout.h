// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Pure hit-testing, highlight geometry, and digit text layout for the tuning bar.
// Hit-testing returns no digit outside the bar or for a zero-width bar.

#pragma once

#include <cmath>
#include <vector>

namespace TunerLayout {

struct TunerBars {
    std::vector<float> top;
    std::vector<float> bottom;
};

inline int digitIndexAt(float mPos, int count, float displayPos, float displayWidth) {
    if (displayWidth == 0.0f) {
        return 0;
    }
    mPos -= 0.5;
    mPos *= 2.0;

    float delta = mPos - displayPos;

    if (delta < 0 || delta > displayWidth) {
        return 0;
    }

    int index = floor((delta / displayWidth) * (count));

    return count - index;
}

inline TunerBars tunerBarQuads(int start, int end, int count, float displayPos, float displayWidth, bool top, bool bottom) {
    TunerBars bars;
    float ofs = (displayWidth / (float)count);
    float p2 = displayPos + ofs * (float)(count - start + 1);
    float p1 = displayPos + ofs * (float)(count - end);
    if (top) {
        bars.top = {p2, 1.0f, p1, 1.0f, p1, 0.0f, p2, 0.0f};
    }
    if (bottom) {
        bars.bottom = {p2, 0.0f, p1, 0.0f, p1, -1.0f, p2, -1.0f};
    }
    return bars;
}

inline std::vector<float> tunerDigitBox(int index, int count, float displayPos, float displayWidth, float viewHeight) {
    float pixelHeight = 2.0 / viewHeight;
    float xpos = displayPos + (displayWidth / (float)count) * (float)(count - index);
    float xpos2 = displayPos + (displayWidth / (float)count) * (float)((count - 1) - index);
    return {xpos, 1.0f - pixelHeight, xpos, -1.0f + pixelHeight, xpos2, -1.0f + pixelHeight, xpos2, 1.0f - pixelHeight,
        xpos, 1.0f - pixelHeight};
}

inline std::vector<float> dragBarQuads(float p1, float p2) {
    return {-1.0f + p2 * 2.0f, 1.0f, -1.0f + p1 * 2.0f, 1.0f, -1.0f + p1 * 2.0f, 0.0f, -1.0f + p2 * 2.0f, 0.0f,
        -1.0f + p2 * 2.0f, 0.0f, -1.0f + p1 * 2.0f, 0.0f, -1.0f + p1 * 2.0f, -1.0f, -1.0f + p2 * 2.0f, -1.0f};
}


struct TunerChar {
    float x = 0.0f;
    int charIndex = 0;
};

inline int tunerFontSize(float viewWidthPx, float viewHeightPx) {
    int fontSize = 32;
    if (viewWidthPx < 300) {
        fontSize = 18;
    } else if (viewWidthPx < 500) {
        fontSize = 24;
    }
    if (viewHeightPx < 18) {
        fontSize = 12;
    } else if (viewHeightPx < 24) {
        fontSize = 16;
    } else if (viewHeightPx < 28) {
        fontSize = 18;
    }
    return fontSize;
}

inline std::vector<TunerChar> tunerChars(int count, int numChars, float displayPos, float displayWidth) {
    std::vector<TunerChar> chars;
    int ofs = count - numChars;
    for (int i = ofs; i < count; i++) {
        TunerChar ch;
        ch.x = displayPos + (displayWidth / (float)count) * (float)i + ((displayWidth / 2.0) / (float)count);
        ch.charIndex = i - ofs;
        chars.push_back(ch);
    }
    return chars;
}

inline std::vector<float> tunerGridVerts(int count, float displayPos, float displayWidth) {
    std::vector<float> gridVerts;
    for (int i = count; i >= 0; i--) {
        float xpos = displayPos + (displayWidth / (float)count) * (float)i;
        gridVerts.push_back(xpos);
        gridVerts.push_back(-1.0);
        gridVerts.push_back(xpos);
        gridVerts.push_back(1.0);
    }
    return gridVerts;
}
}
