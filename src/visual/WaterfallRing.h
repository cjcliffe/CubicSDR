// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cstddef>
#include <vector>

// Headless waterfall ring assembly: quantizes spectrum lines, buffers them,
// and describes texture uploads newest-first with wrap handling. Pure memory
// math, no GL, no I/O: the panel executes the returned upload plan with real
// texture calls, so the whole schedule is unit-testable. Not thread-safe;
// the caller drains under its queue lock, as the canvas already does.
// Caller protocol: setup() once per geometry, setPoints()/step() per line,
// then planUpload(), one real texture upload per op, commitUpload().
class WaterfallRing {
public:
    WaterfallRing();

    // Size the ring for one FFT line across two half-width textures of
    // numLines rows; clears buffered lines but preserves readiness.
    void setup(unsigned int fftSize, int numLines);
    // Stage one spectrum line. Interleaved (x, y) pairs keep the odd elements
    // (magnitudes); plain magnitude arrays pass through as-is.
    void setPoints(const std::vector<float> &points);

    // Buffer one quantized line. Returns false (buffering nothing) while the
    // panel has not marked textures ready, or when no usable points are set.
    bool step();

    struct UploadOp {
        int texture;      // 0 or 1 (left/right spectrum half)
        int y;            // first texture row to write
        int height;       // row count
        size_t srcOffset; // byte offset into reversedLine(texture)
    };

    struct UploadPlan {
        std::vector<UploadOp> ops;
        int rowOffset;  // ring row holding the newest line once committed; the panel scrolls the view to it
        int consumed;   // buffered lines the upload absorbs
    };

    // Reverse buffered lines newest-first and describe the upload, capped at
    // one full texture cycle; older lines are absorbed by the commit.
    UploadPlan planUpload();
    // Adopt the plan: move the ring offset to the newest line and drain the
    // absorbed lines. An empty plan (no buffered lines) is a no-op.
    void commitUpload(const UploadPlan &plan);

    // Marked by the panel once its textures exist; steps before that
    // buffer nothing, mirroring the old texture gate.
    void setReady(bool ready);
    // Newest-first upload bytes for one spectrum half (0 or 1). Only valid
    // after planUpload() with buffered lines; without staged lines there is
    // no backing store and the pointer must not be used.
    const unsigned char *reversedLine(int texture) const;
    // Lines accumulated since the last commit.
    int bufferedLines() const;
    // Ring row holding the newest line; the panel maps it to the top of the view.
    int rowOffset() const;

private:
    unsigned int fftSize; // bins per spectrum line; each texture holds half
    int numLines;         // texture rows; ring capacity in lines
    bool ready;           // panel textures exist and accept uploads
    std::vector<float> points;          // staged magnitudes for the next line
    std::vector<unsigned char> slice;   // quantized half-line scratch
    std::vector<unsigned char> lineBuffer[2];  // accumulated halves, oldest-first
    std::vector<unsigned char> rLineBuffer[2]; // upload staging, newest-first
    int buffered; // lines accumulated since the last commit
    int offset;   // ring row holding the newest line; counts down, wraps at row 0
};
