// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Carrier squelch state machine: opens when the level exceeds the
// threshold continuously for the open delay (rejecting noise bursts),
// closes when it stays below threshold minus hysteresis for the hang
// time (bridging pauses and fades without chattering). Hovering in
// the hysteresis band drains the hang timer instead of resetting it,
// so noise straddling the close edge trends closed rather than riding.
class SquelchState {
public:
    SquelchState(float openDelayMs = 25.0f, float hangMs = 1000.0f, float hysteresisDb = 6.0f, float hangLeakRatio = 0.25f)
        : openDelay(openDelayMs / 1000.0), hang(hangMs / 1000.0), hyst(hysteresisDb), hangLeak(hangLeakRatio) {
        reset();
    }

    void configure(float openDelayMs, float hangMs, float hysteresisDb, float hangLeakRatio = 0.25f) {
        openDelay = openDelayMs / 1000.0;
        hang = hangMs / 1000.0;
        hyst = hysteresisDb;
        hangLeak = hangLeakRatio;
    }

    void setHysteresis(float hysteresisDb) {
        hyst = hysteresisDb;
    }

    void reset() {
        squelched = true;
        openTimer = 0.0;
        hangTimer = 0.0;
    }

    bool processBlock(double levelDb, double thresholdDb, double dtSeconds) {
        if (squelched) {
            if (levelDb >= thresholdDb) {
                openTimer += dtSeconds;
                if (openTimer >= openDelay) {
                    squelched = false;
                    openTimer = 0.0;
                    hangTimer = 0.0;
                }
            } else {
                openTimer = 0.0;
            }
        } else {
            if (levelDb < thresholdDb - hyst) {
                hangTimer += dtSeconds;
                if (hangTimer >= hang) {
                    squelched = true;
                    openTimer = 0.0;
                    hangTimer = 0.0;
                }
            } else if (levelDb < thresholdDb) {
                hangTimer -= hangLeak * dtSeconds;
                if (hangTimer < 0.0) {
                    hangTimer = 0.0;
                }
            } else {
                hangTimer = 0.0;
            }
        }
        return squelched;
    }

    bool isSquelched() const {
        return squelched;
    }

private:
    double openDelay;
    double hang;
    double hyst;
    double hangLeak;
    bool squelched;
    double openTimer;
    double hangTimer;
};

// Raised-cosine mute fade that replaces hard buffer dropping: gain slews
// between muted and full level over the fade time, reversing cleanly
// mid-ramp, so squelch transitions do not click.
class SquelchFade {
public:
    SquelchFade(double sampleRate = 48000.0, float fadeMs = 5.0f)
        : gain(0.0f), phase(1.0f), rampTarget(0.0f), ramping(false) {
        setSampleRate(sampleRate, fadeMs);
    }

    void setSampleRate(double sampleRate, float fadeMs = 5.0f) {
        size_t n = (size_t)(sampleRate * fadeMs / 1000.0);
        fadeSamples = n < 1 ? 1 : n;
    }

    void reset(bool squelched) {
        gain = squelched ? 0.0f : 1.0f;
        rampTarget = gain;
        phase = 1.0f;
        ramping = false;
    }

    // Applies the fade in place; returns true when the block is fully
    // silent and safe to drop.
    bool processBlock(float *data, size_t n, bool squelched) {
        float target = squelched ? 0.0f : 1.0f;
        if (!ramping && gain == target) {
            return gain == 0.0f;
        }
        if (target != rampTarget) {
            rampTarget = target;
            phase = 1.0f - phase;
            ramping = true;
        }
        for (size_t i = 0; i < n; i++) {
            phase += 1.0f / (float)fadeSamples;
            if (phase >= 1.0f) {
                phase = 1.0f;
                ramping = false;
                gain = rampTarget;
            } else {
                float g = 0.5f - 0.5f * cosf((float)M_PI * phase);
                gain = (rampTarget == 1.0f) ? g : (1.0f - g);
            }
            data[i] *= gain;
        }
        return !ramping && gain == 0.0f;
    }

    float currentGain() const {
        return gain;
    }

private:
    size_t fadeSamples;
    float gain;
    float phase;
    float rampTarget;
    bool ramping;
};

// Right-click threshold snap: place the threshold a fixed margin above the
// visible level so noise stays muted while weak signals still break through.
// The default margin matches the close hysteresis, which then reaches back
// down to the level the user clicked on.
inline float snapSquelchAbove(float levelDb, float minDb, float maxDb, float offsetDb = 6.0f) {
    return std::min(maxDb, std::max(minDb, levelDb + offsetDb));
}

// Short-term level spread estimator: a slow average against a fast peak.
// Quiet channels read near zero; noisy channels and live signals read wide.
// Drives the adaptive hysteresis.
class LevelSpread {
public:
    LevelSpread(float avgTauSec = 8.0f, float peakReleaseSec = 1.5f)
        : avgTau(avgTauSec), peakRelease(peakReleaseSec), avg(0.0f), peak(0.0f), seeded(false) {}

    float process(float levelDb, float dtSeconds) {
        if (!seeded) {
            avg = peak = levelDb;
            seeded = true;
            return 0.0f;
        }
        if (dtSeconds > 0.0f) {
            avg += (levelDb - avg) * std::min(1.0f, dtSeconds / avgTau);
            if (levelDb > peak) {
                peak = levelDb;
            } else {
                peak += (levelDb - peak) * std::min(1.0f, dtSeconds / peakRelease);
            }
        }
        float spread = peak - avg;
        return spread > 0.0f ? spread : 0.0f;
    }

    float getAverage() const {
        return avg;
    }

    float getPeak() const {
        return peak;
    }

    void reset() {
        seeded = false;
    }

private:
    float avgTau, peakRelease, avg, peak;
    bool seeded;
};

// Adaptive hysteresis from measured spread: tight on quiet channels so weak
// signals break through, opening to the full band on noisy ones and signals.
inline float spreadHysteresis(float spreadDb, float factor = 1.5f, float minDb = 2.0f, float maxDb = 6.0f) {
    return std::min(maxDb, std::max(minDb, spreadDb * factor));
}

// Solo focus rule: a new break steals focus only into an empty or silent
// slot, never from a live stream. Gated on solo mode with an idle user.
inline bool squelchShouldSteal(bool soloMode, bool userBusy, bool hasActive, bool activeBreaking) {
    return soloMode && !userBusy && (!hasActive || !activeBreaking);
}
