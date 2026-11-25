// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Pure hover-target picking for demodulators under the waterfall cursor.
// An edge-distance match can override a nearer center; the keep-previous zone
// only triggers for degenerate bandwidth.

#pragma once

#include <cstdlib>
#include <vector>

#include "DemodMarkers.h"

namespace WaterfallHover {

enum class DragZone { None, Frequency, BandwidthLeft, BandwidthRight, KeepPrevious };

struct HoverCandidate {
    long long freqHz = 0;
    int bandwidthHz = 0;
    DemodMarkers::Sideband sideband = DemodMarkers::Sideband::Both;
};

struct HoverPick {
    int index = -1;
    DragZone zone = DragZone::None;
};

inline HoverPick pickHover(const std::vector<HoverCandidate> &candidates, long long freqPosHz, long long viewBandwidthHz,
        long long deviceRateHz) {
    HoverPick pick;
    long nearDist = viewBandwidthHz;
    int hover = -1;

    for (size_t i = 0; i < candidates.size(); i++) {
        const HoverCandidate &demod = candidates[i];
        long long freqDiff = demod.freqHz - freqPosHz;
        long halfBw = (demod.bandwidthHz / 2);
        long dist = std::abs(freqDiff);
        double bufferBw = 10000.0 * ((double)viewBandwidthHz / (double)deviceRateHz);
        double maxDist = ((double)halfBw + bufferBw);

        if ((double)dist <= maxDist) {
            if ((freqDiff > 0 && demod.sideband == DemodMarkers::Sideband::UpperOnly) ||
                (freqDiff < 0 && demod.sideband == DemodMarkers::Sideband::LowerOnly)) {
                continue;
            }

            if (dist < nearDist) {
                hover = (int)i;
                nearDist = dist;
            }

            long edgeDist = std::abs(halfBw - dist);
            if (edgeDist < nearDist) {
                hover = (int)i;
                nearDist = edgeDist;
            }
        }
    }

    if (hover < 0) {
        return pick;
    }

    pick.index = hover;
    const HoverCandidate &found = candidates[(size_t)hover];
    long long freqDiff = found.freqHz - freqPosHz;

    if (std::abs(freqDiff) > (found.bandwidthHz / 3)) {
        if (freqDiff > 0) {
            if (found.sideband != DemodMarkers::Sideband::UpperOnly) {
                pick.zone = DragZone::BandwidthLeft;
            } else {
                pick.zone = DragZone::KeepPrevious;
            }
        } else {
            if (found.sideband != DemodMarkers::Sideband::LowerOnly) {
                pick.zone = DragZone::BandwidthRight;
            } else {
                pick.zone = DragZone::KeepPrevious;
            }
        }
    } else {
        pick.zone = DragZone::Frequency;
    }

    return pick;
}

}
