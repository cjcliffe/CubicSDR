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
class WaterfallRing {
public:
    WaterfallRing();

    void setup(unsigned int fftSize, int numLines);
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
        int rowOffset;  // ring row holding the newest line after upload
        int consumed;   // buffered lines the upload absorbs
    };

    // Reverse buffered lines newest-first and describe the upload, capped at
    // one full texture cycle; older lines are absorbed by the commit.
    UploadPlan planUpload();
    void commitUpload(const UploadPlan &plan);

    // Marked by the panel once its textures exist; steps before that
    // buffer nothing, mirroring the old texture gate.
    void setReady(bool ready);
    // Newest-first bytes for one spectrum half. Only valid after planUpload()
    // with buffered lines; empty buffers leave it dangling.
    const unsigned char *reversedLine(int texture) const;
    int bufferedLines() const;
    int rowOffset() const;

private:
    unsigned int fftSize;
    int numLines;
    bool ready;
    std::vector<float> points;
    std::vector<unsigned char> slice;
    std::vector<unsigned char> lineBuffer[2];
    std::vector<unsigned char> rLineBuffer[2];
    int buffered;
    int offset;
};
