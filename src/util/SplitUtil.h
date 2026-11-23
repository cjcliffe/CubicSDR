// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

inline float splitStoredToFraction(float stored, int spanPx) {
    if (stored < 0.0f || spanPx <= 0) {
        return -1.0f;
    }
    if (stored > 1.0f) {
        float frac = stored / (float)spanPx;
        return frac > 1.0f ? 1.0f : frac;
    }
    return stored;
}

inline int splitFractionToPixels(float frac, int spanPx) {
    if (frac < 0.0f || spanPx <= 0) {
        return -1;
    }
    if (frac > 1.0f) {
        frac = 1.0f;
    }
    return (int)(frac * (float)spanPx + 0.5f);
}
