// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cmath>

inline unsigned int scopeSpectrumOutSize(unsigned int binCount, int sampleRate, int inputRate) {
    if (binCount == 0 || inputRate <= 0) {
        return binCount;
    }
    if (sampleRate == inputRate) {
        return binCount;
    }
    double scaled = (double)binCount * ((double)sampleRate / (double)inputRate);
    if (scaled >= (double)binCount) {
        return binCount;
    }
    if (scaled < 1.0) {
        return 1;
    }
    return (unsigned int)floor(scaled);
}

inline double scopePointX(size_t index, size_t count) {
    if (count <= 1) {
        return 0.0;
    }
    return (double)index / (double)(count - 1);
}
