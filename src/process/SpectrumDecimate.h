// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cstddef>
#include <vector>

// Peak reduction over one pixel's bin indexes; out-of-range indexes read the floor.
inline double reducePixelMax(const std::vector<double> &bins, const std::vector<unsigned int> &idxs, double floorVal) {
    double m = floorVal;
    for (unsigned int idx : idxs) {
        if (idx > 0 && idx < bins.size() && bins[idx] > m) {
            m = bins[idx];
        }
    }
    return m;
}
