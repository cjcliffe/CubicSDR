// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "WaterfallPacer.h"

WaterfallPacer::WaterfallPacer() : lps(0), credit(0.0) {
}

void WaterfallPacer::setLinesPerSecond(int linesPerSecond) {
    lps = linesPerSecond;
}

int WaterfallPacer::getLinesPerSecond() const {
    return lps;
}

WaterfallPacer::Plan WaterfallPacer::plan(double dtSeconds, size_t queueDepth) {
    // Bank elapsed time even while paused. Resuming then takes one
    // capped burst on the first frame while the forgiveness step below
    // wipes the leftover credit instead of fast-forwarding through it.
    credit += dtSeconds;

    Plan result{ 0, 0, 0 };
    if (lps <= 0) {
        return result;
    }

    double targetVis = 1.0 / (double)lps;

    // Cap insertions per frame at roughly one frame's share plus one banked
    // line, so the ceiling scales with the requested line rate instead of
    // throttling it. Anything above the cap stays queued as a small cushion
    // against producer jitter instead of jumping at once.
    // Roughly one frame's share of lines plus one banked line, floored
    // at three so very low rates still make progress.
    int maxSteps = (lps + 19) / 20 + 1;
    if (maxSteps < 3) {
        maxSteps = 3;
    }
    size_t maxBacklog = (size_t)maxSteps * 3;

    // Bound latency: drop stale backlog beyond a short window so a transient
    // stall delays the crawl instead of fast-forwarding it.
    if (queueDepth > maxBacklog) {
        result.dropBefore = (int)(queueDepth - maxBacklog);
        queueDepth = maxBacklog;
    }

    int due = (int)(credit / targetVis);
    result.insert = due;
    if (result.insert > maxSteps) {
        result.insert = maxSteps;
    }
    if (result.insert > (int)queueDepth) {
        result.insert = (int)queueDepth;
    }
    credit -= (double)result.insert * targetVis;
    queueDepth -= (size_t)result.insert;

    // Forgive banked catch-up credit instead of bursting: after a stall the
    // crawl resumes at pace on the newest lines rather than jumping.
    // Only forgive when there is something to show for it; an empty
    // queue keeps its banked credit for the refill burst.
    if (credit >= targetVis && queueDepth > 0) {
        credit -= targetVis * (double)(int)(credit / targetVis);
        size_t halfWindow = maxBacklog / 2;
        if (queueDepth > halfWindow) {
            result.dropAfter = (int)(queueDepth - halfWindow);
        }
    }

    return result;
}
