// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "sdr/SDRPacer.h"

#include <climits>

namespace {
constexpr double FRAMES_PER_SECOND = 60.0;
// About a second of backlog; deeper queues drop oldest first.
constexpr int DEFAULT_CAPACITY = 64;
// Catch-up bound: a late tick may emit several frames, but a long stall
// never repays as an unbounded burst.
constexpr double MAX_BURST_FRAMES = 4.0;
}

SDRPacer::SDRPacer()
    : capacity(DEFAULT_CAPACITY), bypass(false), credit(0.0), lastSampleRate(-1), lastNumChannels(-1), lastFrequency(-1) {
}

void SDRPacer::reset() {
    credit = 0.0;
    lastSampleRate = -1;
    lastNumChannels = -1;
    lastFrequency = -1;
}

bool SDRPacer::noteBlock(long long sampleRate, int numChannels, long long frequency) {
    // First block adopts the stream; later ones only report.
    if (lastSampleRate < 0) {
        lastSampleRate = sampleRate;
        lastNumChannels = numChannels;
        lastFrequency = frequency;
        return false;
    }
    return sampleRate != lastSampleRate || numChannels != lastNumChannels || frequency != lastFrequency;
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
    // Clamp absurd depths instead of wrapping the cast below; the drop
    // loop is still bounded by real pops.
    if (queueDepth > static_cast<size_t>(INT_MAX)) {
        queueDepth = static_cast<size_t>(INT_MAX);
    }
    int depth = static_cast<int>(queueDepth);
    // Shed oldest first so a slow consumer sees live frames, not lag.
    int drop = 0;
    if (depth > capacity) {
        drop = depth - capacity;
        depth -= drop;
    }
    if (bypass) {
        return Plan{depth, drop, 0.0};
    }
    // Bank elapsed time as emit allowance. The cap forgives timer
    // overshoot (a late tick still earns its frames) while bounding
    // catch-up after a stall.
    if (dtSeconds > 0.0) {
        credit += dtSeconds * FRAMES_PER_SECOND;
        if (credit > MAX_BURST_FRAMES) {
            credit = MAX_BURST_FRAMES;
        }
    }
    if (depth > 0 && credit >= 1.0) {
        // Emit up to the banked allowance: on-time ticks emit one frame
        // each while a late tick catches up several at once.
        int emit = static_cast<int>(credit);
        if (emit > depth) {
            emit = depth;
        }
        // Spend only what was emitted; the remainder carries forward.
        credit -= emit;
        return Plan{emit, drop, 0.0};
    }
    double waitSeconds = 0.0;
    if (depth > 0) {
        waitSeconds = (1.0 - credit) / FRAMES_PER_SECOND;
    }
    // Dry queues report zero wait so the caller blocks on input instead
    // of sleeping through the arrival of the next block.
    return Plan{0, drop, waitSeconds};
}
