// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/SplitUtil.h"

TEST_SUITE("SplitUtil") {
    TEST_CASE("fractions pass through unchanged") {
        CHECK(splitStoredToFraction(0.0f, 600) == 0.0f);
        CHECK(splitStoredToFraction(0.25f, 1024) == 0.25f);
        CHECK(splitStoredToFraction(1.0f, 600) == 1.0f);
    }

    TEST_CASE("legacy pixels convert against the span") {
        CHECK(splitStoredToFraction(256.0f, 1024) == 0.25f);
        CHECK(splitStoredToFraction(211.0f, 1024) == doctest::Approx(211.0f / 1024.0f));
    }

    TEST_CASE("out-of-range values clamp or stay unset") {
        CHECK(splitStoredToFraction(900.0f, 600) == 1.0f);
        CHECK(splitStoredToFraction(-1.0f, 600) == -1.0f);
        CHECK(splitStoredToFraction(0.5f, 0) == -1.0f);
        CHECK(splitStoredToFraction(0.5f, -3) == -1.0f);
    }

    TEST_CASE("fractions convert back to pixels") {
        CHECK(splitFractionToPixels(0.5f, 600) == 300);
        CHECK(splitFractionToPixels(0.0f, 600) == 0);
        CHECK(splitFractionToPixels(1.0f, 600) == 600);
        CHECK(splitFractionToPixels(1.5f, 600) == 600);
        CHECK(splitFractionToPixels(-1.0f, 600) == -1);
        CHECK(splitFractionToPixels(0.5f, 0) == -1);
    }

    TEST_CASE("save and restore round-trips are stable") {
        const float fracs[] = {0.0f, 0.1f, 0.27f, 0.5f, 0.9f, 1.0f};
        const int spans[] = {211, 600, 1040, 1280, 1920};
        for (float frac : fracs) {
            for (int span : spans) {
                int px = splitFractionToPixels(frac, span);
                float saved = (float)px / (float)span;
                CHECK(splitFractionToPixels(saved, span) == px);
            }
        }
    }
}
