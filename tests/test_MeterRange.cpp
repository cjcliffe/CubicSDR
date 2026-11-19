// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "demod/MeterRange.h"

#include <utility>

TEST_SUITE("MeterRange") {
    TEST_CASE("expands toward strong signal at limited rate") {
        MeterRange m;
        std::pair<float, float> r = m.process(-85.0f, -60.0f, -40.0f, -70.0f, 0.008f, false);
        CHECK(r.first == doctest::Approx(-84.52f).epsilon(0.001));
        CHECK(r.second == doctest::Approx(-59.472f).epsilon(0.001));
    }

    TEST_CASE("contracts slowly toward quiet signal") {
        MeterRange m;
        std::pair<float, float> r = m.process(-100.0f, 0.0f, -70.0f, -70.0f, 0.008f, false);
        CHECK(r.first == doctest::Approx(-99.7f).epsilon(0.001));
        CHECK(r.second == doctest::Approx(-0.816f).epsilon(0.01));
        float floor = -100.0f, ceil = 0.0f;
        for (int b = 0; b < 100; b++) {
            r = m.process(floor, ceil, -70.0f, -70.0f, 0.008f, false);
            floor = r.first;
            ceil = r.second;
        }
        CHECK(floor == doctest::Approx(-82.47f).epsilon(0.05));
        CHECK(ceil == doctest::Approx(-47.68f).epsilon(0.05));
    }

    TEST_CASE("hold freezes slew but constraints still apply") {
        MeterRange m;
        std::pair<float, float> r = m.process(-75.0f, -65.0f, -40.0f, -50.0f, 0.008f, true);
        CHECK(r.first == doctest::Approx(-75.0f));
        CHECK(r.second == doctest::Approx(-48.0f));
    }

    TEST_CASE("minimum span enforced") {
        MeterRange m;
        std::pair<float, float> r = m.process(-70.0f, -65.0f, -67.5f, -100.0f, 0.008f, false);
        CHECK(r.first == doctest::Approx(-70.06f).epsilon(0.001));
        CHECK(r.second == doctest::Approx(-50.06f).epsilon(0.001));
    }

    TEST_CASE("silence clamps rails and settles") {
        MeterRange m;
        std::pair<float, float> r = m.process(-75.0f, -65.0f, -150.0f, -100.0f, 0.008f, false);
        CHECK(r.first == doctest::Approx(-75.6f).epsilon(0.001));
        CHECK(r.second == doctest::Approx(-55.6f).epsilon(0.001));
        float floor = -75.0f, ceil = -65.0f;
        for (int b = 0; b < 20000; b++) {
            r = m.process(floor, ceil, -150.0f, -100.0f, 0.008f, false);
            floor = r.first;
            ceil = r.second;
        }
        CHECK(floor == doctest::Approx(-100.0f).epsilon(0.01));
        CHECK(ceil == doctest::Approx(-80.0f).epsilon(0.01));
    }

    TEST_CASE("threshold stays visible") {
        MeterRange m;
        std::pair<float, float> r = m.process(-75.0f, -65.0f, -70.0f, -40.0f, 0.008f, false);
        CHECK(r.first == doctest::Approx(-75.0f));
        CHECK(r.second == doctest::Approx(-38.0f));
    }
}
