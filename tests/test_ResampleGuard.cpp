// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "demod/ResampleGuard.h"

#include <limits>

TEST_SUITE("ResampleGuard") {
    TEST_CASE("in-range ratios pass through") {
        CHECK(clampResampleRatio(1.0) == 1.0);
        CHECK(clampResampleRatio(0.5) == 0.5);
        CHECK(clampResampleRatio(2.0) == 2.0);
        CHECK(clampResampleRatio(0.001) == 0.001);
        CHECK(clampResampleRatio(100.0) == 100.0);
    }

    TEST_CASE("tiny ratios clamp to the stage floor") {
        CHECK(clampResampleRatio(1e-5) == 1.0 / 4096.0);
        CHECK(clampResampleRatio(0.0001) == 1.0 / 4096.0);
    }

    TEST_CASE("huge ratios clamp to the stage ceiling") {
        CHECK(clampResampleRatio(10000.0) == 4096.0);
        CHECK(clampResampleRatio(1e9) == 4096.0);
    }

    TEST_CASE("degenerate ratios map to the floor") {
        CHECK(clampResampleRatio(0.0) == 1.0 / 4096.0);
        CHECK(clampResampleRatio(-3.0) == 1.0 / 4096.0);
        CHECK(clampResampleRatio(std::numeric_limits<double>::quiet_NaN()) == 1.0 / 4096.0);
    }

    TEST_CASE("bandwidth floor rounds the stage budget up") {
        CHECK(minResampleBandwidth(2000000) == 489);
        CHECK(minResampleBandwidth(4096) == 1);
        CHECK(minResampleBandwidth(4097) == 2);
        CHECK(minResampleBandwidth(10000000) == 2442);
        CHECK(minResampleBandwidth(0) == 0);
        CHECK(minResampleBandwidth(-8) == 0);
    }
}
