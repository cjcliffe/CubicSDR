// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "WaterfallRing.h"

#include <cmath>
#include <cstring>

WaterfallRing::WaterfallRing() : fftSize(0), numLines(0), ready(false), buffered(0), offset(0) {
}

void WaterfallRing::setup(unsigned int fftSize_in, int numLines_in) {
    fftSize = fftSize_in;
    numLines = numLines_in;
    points.resize(fftSize);
    slice.assign(fftSize / 2, 0);
    lineBuffer[0].clear();
    lineBuffer[1].clear();
    rLineBuffer[0].clear();
    rLineBuffer[1].clear();
    buffered = 0;
    // Start at the last row; each upload moves the newest-line row one step
    // down in row index, wrapping from row 0 back to the last row.
    offset = (numLines > 0) ? numLines - 1 : 0;
    // setup() resets geometry and buffered lines but preserves readiness:
    // only the panel's texture creation marks the ring ready, and a later
    // re-setup must not stall a live view back to unready.
}

void WaterfallRing::setPoints(const std::vector<float> &points_in) {
    size_t halfPts = points_in.size() / 2;
    if (halfPts == fftSize) {
        // Interleaved (x, y) pairs from the spectrum processor: magnitudes
        // live at the odd indexes, positions at the even ones.
        for (unsigned int i = 0; i < fftSize; i++) {
            points[i] = points_in[i * 2 + 1];
        }
    } else {
        // Plain magnitude array from any other producer.
        points.assign(points_in.begin(), points_in.end());
    }
}

bool WaterfallRing::step() {
    unsigned int half = fftSize / 2;
    if (!ready || slice.size() != half) {
        return false;
    }

    if (points.empty() || points.size() != fftSize) {
        return false;
    }

    // Left half first (j == 0), then right; quantize magnitudes to bytes.
    for (int j = 0; j < 2; j++) {
        for (unsigned int i = 0, iMax = half; i < iMax; i++) {
            float v = points[(size_t)j * half + i];

            // Clamp out-of-range magnitudes, then scale to a byte value.
            float wv = v < 0 ? 0 : (v > 0.99 ? 0.99 : v);

            slice[i] = (unsigned char)floor(wv * 255.0);
        }

        // Grow both buffers for the appended line; the reversed staging is
        // resized alongside so the upload path never reallocates.
        size_t newBufSize = (size_t)half * (size_t)buffered + half;
        if (lineBuffer[j].size() < newBufSize) {
            lineBuffer[j].resize(newBufSize);
            rLineBuffer[j].resize(newBufSize);
        }
        // Append oldest-first; the reversal happens once at upload time.
        memcpy(&lineBuffer[j][(size_t)half * (size_t)buffered], &slice[0], sizeof(unsigned char) * half);
    }
    buffered++;
    return true;
}

WaterfallRing::UploadPlan WaterfallRing::planUpload() {
    // Defaults describe the empty plan: no ops, offset unchanged, nothing
    // consumed, so committing it is a no-op.
    UploadPlan plan;
    plan.rowOffset = offset;
    plan.consumed = 0;

    unsigned int half = fftSize / 2;
    if (half == 0 || numLines <= 0) {
        return plan;
    }

    // Newest lines first so the upload fills downward from the new top.
    for (int i = 0; i < buffered; i++) {
        // Copy line (buffered-1-i): staging row i holds the i-th newest line.
        for (int j = 0; j < 2; j++) {
            memcpy(&rLineBuffer[j][(size_t)i * half],
                   &lineBuffer[j][((size_t)(buffered - 1) * half) - ((size_t)i * half)], sizeof(unsigned char) * half);
        }
    }

    // Upload at most one full texture cycle; older lines could never be
    // visible, so the commit absorbs them.
    int total = buffered;
    int run = (total < numLines) ? total : numLines;

    if (run > 0) {
        // Ring row where the newest line lands; rows fill newest-first downward.
        int top = offset - run;
        size_t src = 0;

        if (top < 0) {
            top += numLines;
            // Wrap: the newest chunk fills the high block, the older
            // remainder continues at the low end of the texture.
            int high = numLines - top;
            for (int j = 0; j < 2; j++) {
                UploadOp op;
                op.texture = j;
                op.y = top;
                op.height = high;
                op.srcOffset = 0;
                plan.ops.push_back(op);
            }
            // Advance past the high block for the low remainder.
            src = (size_t)high * half;
            int low = run - high;
            if (low > 0) {
                for (int j = 0; j < 2; j++) {
                    UploadOp op;
                    op.texture = j;
                    op.y = 0;
                    op.height = low;
                    op.srcOffset = src;
                    plan.ops.push_back(op);
                }
            }
        } else {
            for (int j = 0; j < 2; j++) {
                UploadOp op;
                op.texture = j;
                op.y = top;
                op.height = run;
                op.srcOffset = src;
                plan.ops.push_back(op);
            }
        }

        plan.rowOffset = top;
        plan.consumed = total;
    }

    return plan;
}

void WaterfallRing::commitUpload(const UploadPlan &plan) {
    // Adopt the new top row and drain every line the upload absorbed,
    // including any over-capacity lines the plan skipped.
    offset = plan.rowOffset;
    buffered -= plan.consumed;
}

void WaterfallRing::setReady(bool ready_in) {
    ready = ready_in;
}

const unsigned char *WaterfallRing::reversedLine(int texture) const {
    return rLineBuffer[texture].data();
}

int WaterfallRing::bufferedLines() const {
    return buffered;
}

int WaterfallRing::rowOffset() const {
    return offset;
}
