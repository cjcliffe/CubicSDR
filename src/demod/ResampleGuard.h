// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

// liquid-dsp's msresamp decomposes a ratio into 2x halfband stages plus an
// arbitrary stage. Ratios needing more than MAX_RESAMPLE_HALFBAND_STAGES
// starve the per-stage prototype designs (older liquid-dsp fails them and
// hands back NULL, the GH#1052 segfault); more than 16 stages is a hard
// liquid error. Clamp both directions; garbage maps to the floor.
#define MAX_RESAMPLE_HALFBAND_STAGES 12

inline double clampResampleRatio(double ratio) {
    const double lo = 1.0 / (1 << MAX_RESAMPLE_HALFBAND_STAGES);
    const double hi = (double)(1 << MAX_RESAMPLE_HALFBAND_STAGES);
    if (!(ratio > 0.0)) {
        return lo;
    }
    if (ratio < lo) {
        return lo;
    }
    if (ratio > hi) {
        return hi;
    }
    return ratio;
}

inline long long minResampleBandwidth(long long sampleRate) {
    if (sampleRate <= 0) {
        return 0;
    }
    return (sampleRate + ((1LL << MAX_RESAMPLE_HALFBAND_STAGES) - 1)) >> MAX_RESAMPLE_HALFBAND_STAGES;
}
