// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "viewmodel/TunerLayout.h"

using namespace TunerLayout;

TEST_SUITE("TunerLayout") {
    TEST_CASE("digit index maps mouse position to digit") {
        float w = (1.0f / 3.0f) * 2.0f;
        CHECK(digitIndexAt(0.0f, 11, -1.0f, w) == 11);
        CHECK(digitIndexAt(0.1f, 11, -1.0f, w) == 8);
        CHECK(digitIndexAt(0.32f, 11, -1.0f, w) == 1);
        CHECK(digitIndexAt(0.5f, 7, -0.25f, 0.5f) == 4);
    }

    TEST_CASE("digit index misses outside the bar") {
        float w = (1.0f / 3.0f) * 2.0f;
        CHECK(digitIndexAt(0.34f, 11, -1.0f, w) == 0);
        CHECK(digitIndexAt(1.0f, 11, -1.0f, w) == 0);
        CHECK(digitIndexAt(-0.5f, 11, -1.0f, w) == 0);
        CHECK(digitIndexAt(0.1f, 11, -1.0f, 0.0f) == 0);
    }

    TEST_CASE("bar quads span the digit range") {
        float w = (1.0f / 3.0f) * 2.0f;
        TunerBars bars = tunerBarQuads(1, 11, 11, -1.0f, w, true, true);
        CHECK(bars.top.size() == 8);
        CHECK(bars.bottom.size() == 8);
        CHECK(bars.top[0] == doctest::Approx(-1.0 / 3.0).epsilon(1e-5));
        CHECK(bars.top[2] == -1.0f);
        CHECK(bars.top[0] > bars.top[2]);
        CHECK(bars.top[1] == 1.0f);
        CHECK(bars.top[5] == 0.0f);
        CHECK(bars.bottom[1] == 0.0f);
        CHECK(bars.bottom[5] == -1.0f);
        TunerBars topOnly = tunerBarQuads(4, 6, 11, -1.0f, w, true, false);
        CHECK(topOnly.top.size() == 8);
        CHECK(topOnly.bottom.empty());
        TunerBars none = tunerBarQuads(4, 6, 11, -1.0f, w, false, false);
        CHECK(none.top.empty());
        CHECK(none.bottom.empty());
    }

    TEST_CASE("digit box outlines one digit") {
        float w = (1.0f / 3.0f) * 2.0f;
        std::vector<float> box = tunerDigitBox(0, 11, -1.0f, w, 100.0f);
        CHECK(box.size() == 10);
        CHECK(box[0] == doctest::Approx(-1.0 / 3.0).epsilon(1e-5));
        CHECK(box[1] == doctest::Approx(0.98).epsilon(1e-5));
        CHECK(box[2] == box[0]);
        CHECK(box[3] == doctest::Approx(-0.98).epsilon(1e-5));
        CHECK(box[4] == doctest::Approx(-1.0f + w / 11.0f * 10.0f).epsilon(1e-5));
        CHECK(box[8] == box[0]);
        CHECK(box[9] == box[1]);
        CHECK(box[1] == box[7]);
        CHECK(box[3] == box[5]);
    }

    TEST_CASE("drag quads span the drag range") {
        std::vector<float> quads = dragBarQuads(0.25f, 0.75f);
        CHECK(quads.size() == 16);
        CHECK(quads[0] == 0.5f);
        CHECK(quads[2] == -0.5f);
        CHECK(quads[1] == 1.0f);
        CHECK(quads[5] == 0.0f);
        CHECK(quads[13] == -1.0f);
    }

    TEST_CASE("tuner font follows viewport") {
        CHECK(tunerFontSize(1024.0f, 100.0f) == 32);
        CHECK(tunerFontSize(200.0f, 100.0f) == 18);
        CHECK(tunerFontSize(400.0f, 100.0f) == 24);
        CHECK(tunerFontSize(300.0f, 100.0f) == 24);
        CHECK(tunerFontSize(1024.0f, 10.0f) == 12);
        CHECK(tunerFontSize(1024.0f, 20.0f) == 16);
        CHECK(tunerFontSize(1024.0f, 26.0f) == 18);
        CHECK(tunerFontSize(1024.0f, 28.0f) == 32);
        CHECK(tunerFontSize(200.0f, 10.0f) == 12);
    }

    TEST_CASE("tuner chars center in cells") {
        std::vector<TunerChar> chars = tunerChars(3, 2, 0.0f, 3.0f);
        CHECK(chars.size() == 2);
        CHECK(chars[0].x == 1.5f);
        CHECK(chars[0].charIndex == 0);
        CHECK(chars[1].x == 2.5f);
        CHECK(chars[1].charIndex == 1);
    }

    TEST_CASE("tuner grid fenceposts cells") {
        std::vector<float> grid = tunerGridVerts(2, 0.0f, 2.0f);
        CHECK(grid.size() == 12);
        CHECK(grid[0] == 2.0f);
        CHECK(grid[1] == -1.0f);
        CHECK(grid[3] == 1.0f);
        CHECK(grid[4] == 1.0f);
        CHECK(grid[8] == 0.0f);
    }
}
