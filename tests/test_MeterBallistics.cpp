// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "demod/MeterBallistics.h"

#include <cmath>

TEST_SUITE("MeterBallistics") {
    TEST_CASE("attack step is larger than release step") {
        MeterBallistics m;
        float attack = m.process(-100.0f, 0.0f, 0.008f) - (-100.0f);
        float release = 0.0f - m.process(0.0f, -100.0f, 0.008f);
        CHECK(attack == doctest::Approx(8.0f).epsilon(0.001));
        CHECK(release == doctest::Approx(1.6f).epsilon(0.001));
        CHECK(attack > release);
    }

    TEST_CASE("attack passes halfway within one time constant") {
        MeterBallistics m;
        float level = -100.0f;
        for (int b = 0; b < 13; b++) {
            level = m.process(level, 0.0f, 0.008f);
        }
        // Closed form of the one-pole recurrence after 13 blocks at k = 0.08.
        float expected = 0.0f - (0.0f - (-100.0f)) * float(std::pow(1.0 - 0.08, 13));
        CHECK(level == doctest::Approx(expected).epsilon(0.01));
        CHECK(level > -50.0f);
        CHECK(level < 0.0f);
    }

    TEST_CASE("converges to target from both directions") {
        MeterBallistics m;
        float rising = -100.0f, falling = 0.0f;
        for (int b = 0; b < 2000; b++) {
            rising = m.process(rising, -20.0f, 0.008f);
            falling = m.process(falling, -20.0f, 0.008f);
        }
        CHECK(rising == doctest::Approx(-20.0f).epsilon(0.001));
        CHECK(falling == doctest::Approx(-20.0f).epsilon(0.001));
    }

    TEST_CASE("dt clamp snaps on huge dt and holds on zero dt") {
        MeterBallistics m;
        CHECK(m.process(-100.0f, 0.0f, 10.0f) == doctest::Approx(0.0f));
        CHECK(m.process(-100.0f, 0.0f, 0.0f) == doctest::Approx(-100.0f));
        CHECK(m.process(-100.0f, 0.0f, -1.0f) == doctest::Approx(-100.0f));
        CHECK(m.process(-50.0f, -50.0f, 0.008f) == doctest::Approx(-50.0f));
    }
}
