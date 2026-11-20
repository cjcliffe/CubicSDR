// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

constexpr double SPECTRUM_WINDOW_PI = 3.141592653589793;

// Hann with coherent-gain correction (x2): windowed tone levels match the unwindowed FFT.
inline void generateHannWindow(std::vector<float> &taps, size_t n) {
    if (n < 2) {
        taps.assign(n, 1.0f);
        return;
    }
    taps.resize(n);
    for (size_t i = 0; i < n; i++) {
        taps[i] = (float)(1.0 - cos(2.0 * SPECTRUM_WINDOW_PI * (double)i / (double)(n - 1)));
    }
}
