// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "CubicSDRDefs.h"

// Display FFT size for a sample rate: full base above the channelizer
// range, halved and quartered below it to match the decimated views.
inline int effectiveSpectrumFFT(int baseSize, long long sampleRate) {
    if (sampleRate <= CHANNELIZER_RATE_MAX / 8) {
        return baseSize / 4;
    }
    if (sampleRate <= CHANNELIZER_RATE_MAX) {
        return baseSize / 2;
    }
    return baseSize;
}
