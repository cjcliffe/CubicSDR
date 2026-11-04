// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "sdr/SDRPacer.h"

namespace {
constexpr double FRAMES_PER_SECOND = 60.0;
constexpr int DEFAULT_CAPACITY = 64;
constexpr double MAX_BURST_FRAMES = 4.0;
}

SDRPacer::SDRPacer()
    : capacity(DEFAULT_CAPACITY), bypass(false), credit(0.0), lastSampleRate(-1), lastNumChannels(-1) {
}

void SDRPacer::reset() {
    credit = 0.0;
    lastSampleRate = -1;
    lastNumChannels = -1;
}

bool SDRPacer::noteBlock(long long sampleRate, int numChannels) {
    if (lastSampleRate < 0) {
        lastSampleRate = sampleRate;
        lastNumChannels = numChannels;
        return false;
    }
    return sampleRate != lastSampleRate || numChannels != lastNumChannels;
}

void SDRPacer::setCapacity(int frames) {
    capacity = frames < 1 ? 1 : frames;
}

int SDRPacer::getCapacity() const {
    return capacity;
}

void SDRPacer::setBypass(bool bypass_in) {
    bypass = bypass_in;
}

bool SDRPacer::isBypassed() const {
    return bypass;
}

SDRPacer::Plan SDRPacer::plan(double dtSeconds, size_t queueDepth) {
    int depth = static_cast<int>(queueDepth);
    int drop = 0;
    if (depth > capacity) {
        drop = depth - capacity;
        depth -= drop;
    }
    if (bypass) {
        return Plan{depth, drop, 0.0};
    }
    if (dtSeconds > 0.0) {
        credit += dtSeconds * FRAMES_PER_SECOND;
        if (credit > MAX_BURST_FRAMES) {
            credit = MAX_BURST_FRAMES;
        }
    }
    if (depth > 0 && credit >= 1.0) {
        int emit = static_cast<int>(credit);
        if (emit > depth) {
            emit = depth;
        }
        credit -= emit;
        return Plan{emit, drop, 0.0};
    }
    double waitSeconds = 0.0;
    if (depth > 0) {
        waitSeconds = (1.0 - credit) / FRAMES_PER_SECOND;
    }
    return Plan{0, drop, waitSeconds};
}
