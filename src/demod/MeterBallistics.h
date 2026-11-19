// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <algorithm>

// Asymmetric one-pole smoother for the demodulator signal meter. Attack is
// fast enough to track speech syllables, release is slow so the needle falls
// like an analog S-meter. Display only; the squelch decision reads the raw
// per-block level.
class MeterBallistics {
public:
    MeterBallistics(float attackSeconds = 0.1f, float releaseSeconds = 0.5f)
        : attackSeconds(attackSeconds), releaseSeconds(releaseSeconds) {}

    float process(float level, float target, float dtSeconds) const {
        float tau = (target > level) ? attackSeconds : releaseSeconds;
        float k = (tau > 0.0f) ? (dtSeconds / tau) : 1.0f;
        k = std::min(1.0f, std::max(0.0f, k));
        return level + (target - level) * k;
    }

    float getAttackSeconds() const {
        return attackSeconds;
    }

    float getReleaseSeconds() const {
        return releaseSeconds;
    }

private:
    float attackSeconds, releaseSeconds;
};
