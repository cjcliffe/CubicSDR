// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// NCO lock angle that regenerates an in-phase 38 kHz stereo reference.
// The broadcast subcarrier is sine-phased with the 19 kHz pilot (ITU-R
// BS.450), and doubling the pilot PLL adds the pilot filter phase twice,
// so locking the pilot branch at this angle lands the downconverted
// difference signal on the real axis as +S. The pilot filter phase is
// measured at kit build; see ModemFMStereo::buildKit.
inline float stereoReferencePhase(float pilotFilterPhase) {
    return pilotFilterPhase - (float)(M_PI / 4.0);
}

// Integer-sample delay line that time-aligns the mono path with the
// stereo difference path, whose Hilbert R2C/C2R pair delays the signal.
// Output block n is the input stream delayed by the configured samples,
// continuous across blocks via the history tail.
class MonoDelayLine {
public:
    explicit MonoDelayLine(unsigned delay = 0) {
        setDelay(delay);
    }

    void setDelay(unsigned delay) {
        hist.assign(delay, 0.0f);
    }

    unsigned delay() const {
        return (unsigned)hist.size();
    }

    void reset() {
        std::fill(hist.begin(), hist.end(), 0.0f);
    }

    void apply(const float *input, size_t n, float *output) {
        size_t d = hist.size();
        for (size_t i = 0; i < n; i++) {
            output[i] = (i < d) ? hist[i] : input[i - d];
        }
        if (n >= d) {
            hist.assign(input + (n - d), input + n);
        } else {
            hist.erase(hist.begin(), hist.begin() + n);
            hist.insert(hist.end(), input, input + n);
        }
    }

private:
    std::vector<float> hist;
};

// Pilot-gated stereo blend: fades the difference channel toward mono
// when the 19 kHz pilot is weak or absent so mono and weak stations do
// not play hiss in the stereo difference. Gain slews fast up, slow down.
class StereoBlender {
public:
    StereoBlender(double sampleRate = 200000.0, float attackMs = 20.0f, float releaseMs = 200.0f)
        : power(0.0f), gain(0.0f) {
        setRates(sampleRate, attackMs, releaseMs);
    }

    void setRates(double sampleRate, float attackMs, float releaseMs) {
        smooth = rateCoeff(sampleRate, 10.0);
        attack = rateCoeff(sampleRate, attackMs);
        release = rateCoeff(sampleRate, releaseMs);
    }

    void reset() {
        power = 0.0f;
        gain = 0.0f;
    }

    float process(float vReal, float vImag) {
        float p = vReal * vReal + vImag * vImag;
        power += smooth * (p - power);
        float target;
        if (power <= BLEND_LOW) {
            target = 0.0f;
        } else if (power >= BLEND_HIGH) {
            target = 1.0f;
        } else {
            target = (power - BLEND_LOW) / (BLEND_HIGH - BLEND_LOW);
        }
        float k = (target > gain) ? attack : release;
        gain += k * (target - gain);
        return gain;
    }

    float currentGain() const {
        return gain;
    }

private:
    static float rateCoeff(double sampleRate, double ms) {
        return (float)(1.0 - exp(-1.0 / (sampleRate * ms / 1000.0)));
    }

    // Squared pilot analytic amplitude at the PLL branch: full stereo
    // above |v| ~ 0.063, mono below |v| = 0.02, linear ramp between.
    static constexpr float BLEND_LOW = 0.0004f;
    static constexpr float BLEND_HIGH = 0.004f;

    float power;
    float gain;
    float smooth;
    float attack;
    float release;
};
