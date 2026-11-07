// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cstddef>

// Headless waterfall pacing: decides how many queued spectrum lines to
// insert or discard each frame. Pure arithmetic over elapsed time and queue
// depth: no locks, no I/O, no globals, so the schedule is unit-testable.
class WaterfallPacer {
public:
    WaterfallPacer();

    void setLinesPerSecond(int linesPerSecond);
    int getLinesPerSecond() const;

    // One frame of schedule. dropBefore keeps the queue within the
    // short latency window, insert steps lines into the waterfall, and
    // dropAfter trims whatever is still beyond the half window once
    // banked credit has been forgiven.
    struct Plan {
        int insert;      // queued lines to step into the waterfall
        int dropBefore;  // stale queued lines to discard before inserting
        int dropAfter;   // backlog to trim after inserting
    };

    // Advance the schedule by dtSeconds with queueDepth lines waiting.
    // The caller guards every pop, so a queue that comes up short only
    // shortens the frame.
    Plan plan(double dtSeconds, size_t queueDepth);

private:
    int lps;
    double credit;
};
