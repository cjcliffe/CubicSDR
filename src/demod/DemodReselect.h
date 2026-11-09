// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cstddef>
#include <vector>

// Picks a fallback demodulator selection by frequency when the current
// selection is lost (normally a delete). Order: the previously selected
// demod when it is still in view, else the first demod in view, else
// nothing: an off-screen selection would be worse than none. "First"
// means lowest frequency with lowest-index tiebreak. A non-positive
// span means no viewport, so every candidate counts as in view.
// Stdlib-only so the unit tests drive it without the application.
class DemodReselect {
public:
    // freqs parallels the caller's demodulator list; prevIndex is the
    // previously selected position, or negative when there is none.
    // centerHz/spanHz bound the visible [center-span/2, center+span/2]
    // window, edges inclusive. Returns the picked index, or -1 when
    // empty or nothing is in view.
    static int pick(const std::vector<long long>& freqs, int prevIndex, long long centerHz, long long spanHz) {
        if (freqs.empty()) {
            return -1;
        }

        bool hasPrev = prevIndex >= 0 && (size_t)prevIndex < freqs.size();

        auto inView = [&](size_t i) {
            if (spanHz <= 0) {
                return true;
            }
            long long half = spanHz / 2;
            return freqs[i] >= centerHz - half && freqs[i] <= centerHz + half;
        };

        if (hasPrev && inView((size_t)prevIndex)) {
            return prevIndex;
        }

        int best = -1;
        for (size_t i = 0; i < freqs.size(); i++) {
            if (!inView(i)) {
                continue;
            }
            if (best < 0 || freqs[i] < freqs[best]) {
                best = (int)i;
            }
        }
        return best;
    }
};
