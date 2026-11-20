// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cmath>

// Linear magnitudes below this read as silence in dB.
constexpr double SPECTRUM_DB_FLOOR_MAG = 1e-12;

inline double magToDb(double mag) {
    if (mag < SPECTRUM_DB_FLOOR_MAG) {
        mag = SPECTRUM_DB_FLOOR_MAG;
    }
    return 20.0 * log10(mag);
}

inline double dbToMag(double db) {
    return pow(10.0, db / 20.0);
}

inline double iirStep(double state, double input, double rate) {
    return state + (input - state) * rate;
}
