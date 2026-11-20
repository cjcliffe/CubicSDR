// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

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
