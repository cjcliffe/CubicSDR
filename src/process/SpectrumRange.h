// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cstddef>
#include <vector>

// Bins each side of DC skipped by the auto-range scan: the spike core plus near skirts.
constexpr size_t SPECTRUM_DC_EXCLUDE_HALF_WIDTH = 8;

struct SpectrumRange {
    double ceil;
    double floor;
};

inline SpectrumRange scanSpectrumRange(const std::vector<double> &bins, bool hideDC, size_t dcIndex) {
    SpectrumRange range{0.0, 1.0};
    for (size_t i = 0; i < bins.size(); i++) {
        if (hideDC && i + SPECTRUM_DC_EXCLUDE_HALF_WIDTH >= dcIndex && i <= dcIndex + SPECTRUM_DC_EXCLUDE_HALF_WIDTH) {
            continue;
        }
        if (bins[i] > range.ceil) {
            range.ceil = bins[i];
        }
        if (bins[i] < range.floor) {
            range.floor = bins[i];
        }
    }
    return range;
}
