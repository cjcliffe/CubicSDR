// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

// Steps a TAB-style selection over a frequency-ordered active list.
// count is the list size; current is the selected position, or
// negative (or past the end) when there is no usable selection.
// Forward steps toward higher frequencies and wraps to the front;
// backward steps toward lower frequencies and wraps to the back.
// With no usable selection, forward enters at the front (first in
// view) and backward at the back (last in view). Returns the new
// position, or -1 when the list is empty. Stdlib-only so the unit
// tests drive it without the application.
class DemodCycle {
public:
    static int step(int count, int current, bool forward) {
        if (count <= 0) {
            return -1;
        }
        if (current < 0 || current >= count) {
            return forward ? 0 : count - 1;
        }
        if (forward) {
            return (current + 1) % count;
        }
        return (current + count - 1) % count;
    }
};
