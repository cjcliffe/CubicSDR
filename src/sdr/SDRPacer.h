// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cstddef>

// Headless SDR stream pacing: decides how many queued IQ blocks to emit
// or discard each tick. Pure arithmetic over elapsed time and queue depth:
// no locks, no I/O, no globals, so the schedule is unit-testable.
//
// The gate is fixed at 60Hz to match frame-sized blocks; a different gate
// here than the frame size downstream would drift systematically, so it is
// deliberately not configurable.
class SDRPacer {
public:
    SDRPacer();

    // Backlog bound in frames (floors at 1); deeper queues drop oldest.
    void setCapacity(int frames);
    int getCapacity() const;

    // Bypass forwards everything immediately, ignoring the gate.
    void setBypass(bool bypass);
    bool isBypassed() const;

    // Clears pacing credit and stream tracking.
    void reset();

    // Notes an incoming block's stream. Returns true when it differs from
    // the running one; the caller must then discard everything buffered,
    // reset, and note the block again to adopt the new stream.
    bool noteBlock(long long sampleRate, int numChannels, long long frequency);

    // One tick of schedule. emit counts queued blocks to forward (up to
    // the burst bound, so late ticks catch up instead of losing time;
    // bypassed forwards everything), drop counts stale oldest blocks to
    // discard to stay within capacity, and waitSeconds tells the caller
    // how long until the next emit is due (zero when it should act now
    // or wait on input).
    struct Plan {
        int emit;
        int drop;
        double waitSeconds;
    };

    // Advance the schedule by dtSeconds with queueDepth blocks waiting.
    Plan plan(double dtSeconds, size_t queueDepth);

private:
    int capacity;
    bool bypass;
    // Emit allowance in frames, banked from elapsed time and spent on emit.
    double credit;
    // Running stream identity; a change means the caller must flush.
    long long lastSampleRate;
    int lastNumChannels;
    long long lastFrequency;
};
