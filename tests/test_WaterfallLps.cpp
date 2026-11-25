// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "viewmodel/WaterfallLps.h"

#include <cmath>

TEST_SUITE("WaterfallLps") {
    TEST_CASE("default reset round-trips to 30") {
        CHECK(waterfallMeterToLps(sqrtf(30.0f)) == 30);
    }

    TEST_CASE("integer speeds round-trip exactly") {
        const int speeds[] = { 0, 1, 2, 5, 10, 25, 30, 100, 512, 1023, 1024 };
        for (int lps : speeds) {
            CHECK(waterfallMeterToLps(sqrtf((float)lps)) == lps);
        }
    }

    TEST_CASE("fractional values round half up") {
        CHECK(waterfallMeterToLps(5.5f) == 30);
        CHECK(waterfallMeterToLps(0.0f) == 0);
        CHECK(waterfallMeterToLps(32.0f) == 1024);
    }
}
