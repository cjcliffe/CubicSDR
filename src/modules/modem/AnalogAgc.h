// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Peak-follower AGC with fast attack and slow release. Call processBlock
// once per demod block; it returns the gain normalizing the block peak
// toward the target. Gain is bounded at 60 dB so weak signals reach
// target (volume and squelch manage loudness) without exploding.
class AnalogAgc {
public:
    AnalogAgc(double sampleRate = 5400.0, float attackMs = 2.0f, float releaseMs = 300.0f)
        : level(1.0f), fs(sampleRate), attackMs(attackMs), releaseMs(releaseMs) {
    }

    void setSampleRate(double sampleRate) {
        fs = sampleRate;
    }

    void setBallistics(float attack, float release) {
        attackMs = attack;
        releaseMs = release;
    }

    void reset() {
        level = 1.0f;
    }

    float processBlock(const float *data, size_t n) {
        if (n == 0) {
            return currentGain();
        }
        float peak = 0.0f;
        for (size_t i = 0; i < n; i++) {
            float a = fabsf(data[i]);
            if (a > peak) {
                peak = a;
            }
        }
        return processLevel(peak, n);
    }

    float processLevel(float peak, size_t n) {
        float target = peak > PEAK_FLOOR ? peak : PEAK_FLOOR;
        double tau = (target > level ? attackMs : releaseMs) / 1000.0;
        float k = 1.0f - expf((float)(-(double)n / (fs * tau)));
        level += k * (target - level);
        return currentGain();
    }

    float currentGain() const {
        float g = TARGET_PEAK / (level > PEAK_FLOOR ? level : PEAK_FLOOR);
        return g > MAX_GAIN ? MAX_GAIN : g;
    }

    float currentLevel() const {
        return level;
    }

private:
    static constexpr float TARGET_PEAK = 0.5f;
    static constexpr float MAX_GAIN = 1000.0f;
    static constexpr float PEAK_FLOOR = 1e-6f;

    float level;
    double fs;
    float attackMs;
    float releaseMs;
};

// One-pole DC blocker: y = x - x1 + a * y1. At the default pole the
// corner sits a few Hz at voice rates, removing carrier leak and
// mistuning rumble without touching audio.
class DcBlocker {
public:
    explicit DcBlocker(float pole = 0.995f) : a(pole), x1(0.0f), y1(0.0f) {
    }

    void reset() {
        x1 = 0.0f;
        y1 = 0.0f;
    }

    float process(float x) {
        float y = x - x1 + a * y1;
        x1 = x;
        y1 = y;
        return y;
    }

private:
    float a;
    float x1;
    float y1;
};

// Leaky-resonator tone-level detector. Measures the amplitude of a tone at
// a known frequency while rejecting wideband transients (key clicks) and
// out-of-band noise that would desense a wideband peak follower. Magnitude
// is normalized so a full-scale tone reads its peak amplitude.
class ToneLevelDetector {
public:
    ToneLevelDetector(double sampleRate = 48000.0, double toneHz = 650.0, double bandwidthHz = 150.0)
        : y1(0.0f), y2(0.0f), p(0.0) {
        configure(sampleRate, toneHz, bandwidthHz);
    }

    void configure(double sampleRate, double toneHz, double bandwidthHz = 150.0) {
        double w = 2.0 * M_PI * toneHz / sampleRate;
        double r = 1.0 - M_PI * bandwidthHz / sampleRate;
        if (r < 0.0) {
            r = 0.0;
        }
        if (r > 0.999999) {
            r = 0.999999;
        }
        c = 2.0 * r * cos(w);
        r2 = r * r;
        double cw = cos(w), sw = sin(w);
        double dR = 1.0 - c * cw + r2 * (cw * cw - sw * sw);
        double dI = c * sw - r2 * (2.0 * cw * sw);
        norm = sqrt(dR * dR + dI * dI);
        ke = 1.0 - exp(-1.0 / (sampleRate * 0.01));
    }

    void reset() {
        y1 = 0.0f;
        y2 = 0.0f;
        p = 0.0;
    }

    float measureBlock(const float *data, size_t n) {
        double peak2 = 0.0;
        for (size_t i = 0; i < n; i++) {
            float y = data[i] + (float)c * y1 - (float)r2 * y2;
            y2 = y1;
            y1 = y;
            p += ke * ((double)y * (double)y - p);
            if (p > peak2) {
                peak2 = p;
            }
        }
        return (float)(sqrt(peak2 * 2.0) * norm);
    }

private:
    double c;
    double r2;
    double norm;
    double ke;
    float y1;
    float y2;
    double p;
};

// Amplitude-dB / linear-gain conversion shared by the CW gain setting so
// manual and automatic gain use one convention.
inline float linearGainToDb(float gain) {
    return 20.0f * log10f(gain > 1e-9f ? gain : 1e-9f);
}

inline float dbToLinearGain(float db) {
    return powf(10.0f, db / 20.0f);
}
