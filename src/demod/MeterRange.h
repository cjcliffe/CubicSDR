// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <algorithm>
#include <utility>

// Slow auto-ranging display scale for the demodulator signal meter: the
// rails glide toward the signal (fast to expand, slow to contract) so the
// action spreads across the meter instead of bunching in the middle, while
// the threshold stays visible and the mapping stays put while dragging.
class MeterRange {
public:
    MeterRange(float minDb = -100.0f, float maxDb = 30.0f, float minSpanDb = 20.0f, float headDb = 2.0f, float tailDb = 5.0f,
        float expandPerSec = 3.0f, float contractPerSec = 1.5f)
        : minDb(minDb), maxDb(maxDb), minSpanDb(minSpanDb), headDb(headDb), tailDb(tailDb), expandPerSec(expandPerSec),
        contractPerSec(contractPerSec) {}

    std::pair<float, float> process(float floor, float ceil, float currentDb, float thresholdDb, float dtSeconds, bool hold) const {
        float floorTarget = clamp(currentDb - tailDb, minDb, maxDb);
        float ceilTarget = clamp(currentDb + headDb, minDb, maxDb);
        if (!hold && dtSeconds > 0.0f) {
            // Ceiling rises and floor falls to expand; both reverse to contract.
            ceil = slew(ceil, ceilTarget, dtSeconds, true);
            floor = slew(floor, floorTarget, dtSeconds, false);
        }
        floor = clamp(floor, minDb, maxDb);
        ceil = clamp(ceil, minDb, maxDb);
        float needCeil = thresholdDb + headDb;
        if (needCeil > maxDb) {
            needCeil = maxDb;
        }
        if (ceil < needCeil) {
            ceil = needCeil;
        }
        // Span pushes the ceiling; re-pin both rails if that exceeds the top.
        if (ceil < floor + minSpanDb) {
            ceil = floor + minSpanDb;
        }
        if (ceil > maxDb) {
            ceil = maxDb;
            floor = ceil - minSpanDb;
        }
        if (floor < minDb) {
            floor = minDb;
        }
        return std::make_pair(floor, ceil);
    }

private:
    float clamp(float v, float lo, float hi) const {
        return std::min(hi, std::max(lo, v));
    }

    float slew(float rail, float target, float dtSeconds, bool upIsExpand) const {
        float d = target - rail;
        bool expanding = upIsExpand ? (d > 0.0f) : (d < 0.0f);
        float k = std::min(1.0f, (expanding ? expandPerSec : contractPerSec) * dtSeconds);
        return rail + d * k;
    }

    float minDb, maxDb, minSpanDb, headDb, tailDb, expandPerSec, contractPerSec;
};
