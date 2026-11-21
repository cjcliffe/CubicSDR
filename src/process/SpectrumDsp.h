// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cmath>
#include <cstddef>
#include <vector>
#include "CubicSDRDefs.h"

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

struct DCRange {
    int start;
    int end;
};

// Display-point span covering +/-2 kHz around the DC point, computed in
// floating point: integer Hz-per-point truncation misplaces the patch,
// increasingly so at larger FFT sizes.
inline DCRange dcPatchRange(long long zeroPtHz, long long freqRangeHz, int fftSize) {
    if (freqRangeHz <= 0 || fftSize <= 0) {
        return {0, 0};
    }
    int center = (int)((double)zeroPtHz * (double)fftSize / (double)freqRangeHz);
    int half = (int)(2000.0 * (double)fftSize / (double)freqRangeHz);
    if (half < 1) {
        half = 1;
    }
    return {center - half, center + half};
}
