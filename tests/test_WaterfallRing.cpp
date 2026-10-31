// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "panel/WaterfallRing.h"

namespace {
std::vector<float> constLine(float v, size_t n) {
    return std::vector<float>(n, v);
}
}

TEST_SUITE("WaterfallRing") {
    TEST_CASE("nothing buffers before textures are ready") {
        WaterfallRing ring;
        ring.setup(8, 8);
        ring.setPoints(constLine(0.5f, 8));

        CHECK(ring.bufferedLines() == 0);
        CHECK(ring.step() == false);
        CHECK(ring.bufferedLines() == 0);

        WaterfallRing::UploadPlan plan = ring.planUpload();
        CHECK(plan.ops.empty());
        CHECK(plan.consumed == 0);
    }

    TEST_CASE("points are quantized and clamped to bytes") {
        WaterfallRing ring;
        ring.setup(8, 8);
        ring.setReady(true);
        // Exercises clamping on both ends plus mid-range steps.
        ring.setPoints({ -0.5f, 0.5f, 1.5f, 0.0f, 0.25f, 0.75f, 0.99f, 2.0f });
        CHECK(ring.step() == true);

        WaterfallRing::UploadPlan plan = ring.planUpload();
        CHECK(plan.consumed == 1);
        const unsigned char *left = ring.reversedLine(0);
        CHECK(left[0] == 0);
        CHECK(left[1] == 127);
        CHECK(left[2] == 252);
        CHECK(left[3] == 0);
        const unsigned char *right = ring.reversedLine(1);
        CHECK(right[0] == 63);
        CHECK(right[1] == 191);
        CHECK(right[2] == 252);
        CHECK(right[3] == 252);
    }

    TEST_CASE("interleaved input takes the odd elements") {
        WaterfallRing ring;
        ring.setup(8, 8);
        ring.setReady(true);
        // 16 floats in, 8 points kept: indexes 1, 3, .., 15.
        std::vector<float> in(16, 0.0f);
        in[1] = 0.5f;
        ring.setPoints(in);
        CHECK(ring.step() == true);

        WaterfallRing::UploadPlan plan = ring.planUpload();
        CHECK(plan.consumed == 1);
        const unsigned char *left = ring.reversedLine(0);
        CHECK(left[0] == 127);
        for (int i = 1; i < 4; i++) {
            CHECK(left[i] == 0);
        }
        const unsigned char *right = ring.reversedLine(1);
        for (int i = 0; i < 4; i++) {
            CHECK(right[i] == 0);
        }
    }

    TEST_CASE("size mismatch buffers nothing") {
        WaterfallRing ring;
        ring.setup(8, 8);
        ring.setReady(true);
        ring.setPoints(constLine(0.5f, 6));
        CHECK(ring.step() == false);
        CHECK(ring.bufferedLines() == 0);
    }

    TEST_CASE("buffered lines upload newest-first without wrap") {
        WaterfallRing ring;
        ring.setup(8, 8);
        ring.setReady(true);

        ring.setPoints(constLine(0.0f, 8));
        CHECK(ring.step() == true);
        ring.setPoints(constLine(0.5f, 8));
        CHECK(ring.step() == true);
        ring.setPoints(constLine(0.25f, 8));
        CHECK(ring.step() == true);
        CHECK(ring.bufferedLines() == 3);

        // Offsets start at the last row; three lines land at rows 4..6.
        WaterfallRing::UploadPlan plan = ring.planUpload();
        CHECK(plan.consumed == 3);
        CHECK(plan.rowOffset == 4);
        CHECK(plan.ops.size() == 2);
        CHECK(plan.ops[0].texture == 0);
        CHECK(plan.ops[0].y == 4);
        CHECK(plan.ops[0].height == 3);
        CHECK(plan.ops[0].srcOffset == 0);

        const unsigned char *line = ring.reversedLine(0);
        CHECK(line[0] == 63);
        CHECK(line[4] == 127);
        CHECK(line[8] == 0);

        ring.commitUpload(plan);
        CHECK(ring.bufferedLines() == 0);
        CHECK(ring.rowOffset() == 4);
    }

    TEST_CASE("upload wraps at the ring top and absorbs the overflow") {
        WaterfallRing ring;
        ring.setup(8, 4);
        ring.setReady(true);

        // Six lines at 0.0, 0.1, .., 0.5; only the newest four survive.
        for (int k = 0; k < 6; k++) {
            ring.setPoints(constLine((float)k * 0.1f, 8));
            CHECK(ring.step() == true);
        }
        CHECK(ring.bufferedLines() == 6);

        WaterfallRing::UploadPlan plan = ring.planUpload();
        CHECK(plan.consumed == 6);
        // Newest chunk fills the high block (row 3), the older remainder
        // continues at the low end (rows 0..2).
        CHECK(plan.ops.size() == 4);
        CHECK(plan.ops[0].texture == 0);
        CHECK(plan.ops[0].y == 3);
        CHECK(plan.ops[0].height == 1);
        CHECK(plan.ops[0].srcOffset == 0);
        CHECK(plan.ops[2].texture == 0);
        CHECK(plan.ops[2].y == 0);
        CHECK(plan.ops[2].height == 3);
        CHECK(plan.ops[2].srcOffset == 4);

        const unsigned char *line = ring.reversedLine(0);
        // Newest line first: floor(0.5 * 255) = 127, then 102, 76, 51.
        CHECK(line[0] == 127);
        CHECK(line[4] == 102);
        CHECK(line[8] == 76);
        CHECK(line[12] == 51);

        ring.commitUpload(plan);
        CHECK(ring.bufferedLines() == 0);
        CHECK(ring.rowOffset() == 3);
    }

    TEST_CASE("empty plan commits to a no-op") {
        WaterfallRing ring;
        ring.setup(8, 8);
        ring.setReady(true);

        WaterfallRing::UploadPlan plan = ring.planUpload();
        CHECK(plan.ops.empty());
        ring.commitUpload(plan);
        CHECK(ring.bufferedLines() == 0);
        CHECK(ring.rowOffset() == 7);
    }
}
