// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

// The driver is tuned to the displayed frequency minus the offset
// (upconverters), so the device range clamps that RF value, not the
// display value; the result is shifted back to display domain.
inline long long clampTuneFrequency(long long freq, long long offset, long long rangeMin, long long rangeMax) {
    long long rf = freq - offset;
    if (rf < rangeMin) {
        rf = rangeMin;
    } else if (rf > rangeMax) {
        rf = rangeMax;
    }
    return rf + offset;
}

// Keep a dragged demodulator center inside the visible view (which never
// exceeds the data window); outside the data window the post thread
// deactivates it.
inline long long pinDemodCenter(long long freq, long long viewCenter, long long viewBW) {
    long long half = viewBW / 2;
    if (freq < viewCenter - half) {
        return viewCenter - half;
    }
    if (freq > viewCenter + half) {
        return viewCenter + half;
    }
    return freq;
}

// Keep [viewCenter +/- viewBW/2] inside the data window
// [deviceFreq +/- sampleRate/2]. Views at least as wide as the data
// have no room to pan and collapse to device center. Sub-zero views
// are allowed: the FFT bins exist across the whole window.
inline long long pinViewCenter(long long viewCenter, long long viewBW, long long deviceFreq, long long sampleRate) {
    long long halfSpan = sampleRate / 2 - viewBW / 2;
    if (halfSpan <= 0) {
        return deviceFreq;
    }
    if (viewCenter < deviceFreq - halfSpan) {
        return deviceFreq - halfSpan;
    }
    if (viewCenter > deviceFreq + halfSpan) {
        return deviceFreq + halfSpan;
    }
    return viewCenter;
}
