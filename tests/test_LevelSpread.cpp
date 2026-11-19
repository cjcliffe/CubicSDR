// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "demod/SquelchState.h"

TEST_SUITE("LevelSpread") {
    TEST_CASE("steady level reads zero spread") {
        LevelSpread s;
        float spread = 0.0f;
        for (int b = 0; b < 500; b++) {
            spread = s.process(-70.0f, 0.008f);
        }
        CHECK(spread == doctest::Approx(0.0f));
    }

    TEST_CASE("alternation settles near half the swing") {
        LevelSpread s;
        float spread = 0.0f;
        for (int b = 0; b < 2000; b++) {
            spread = s.process((b % 2 == 0) ? -70.0f : -68.0f, 0.008f);
        }
        CHECK(spread > 0.5f);
        CHECK(spread < 1.5f);
    }

    TEST_CASE("step up jumps then relaxes slowly") {
        LevelSpread s;
        for (int b = 0; b < 200; b++) {
            s.process(-70.0f, 0.008f);
        }
        CHECK(s.process(-50.0f, 0.008f) == doctest::Approx(20.0f).epsilon(0.01));
        float spread = 0.0f;
        for (int b = 0; b < 300; b++) {
            spread = s.process(-50.0f, 0.008f);
        }
        CHECK(spread > 5.0f);
    }

    TEST_CASE("peak decays gradually") {
        LevelSpread s;
        s.process(-50.0f, 0.008f);
        s.process(-70.0f, 0.008f);
        CHECK(s.getPeak() < -50.0f);
        CHECK(s.getPeak() > -51.0f);
    }

    TEST_CASE("hysteresis maps spread to band") {
        CHECK(spreadHysteresis(0.0f) == doctest::Approx(2.0f));
        CHECK(spreadHysteresis(1.0f) == doctest::Approx(2.0f));
        CHECK(spreadHysteresis(2.0f) == doctest::Approx(3.0f));
        CHECK(spreadHysteresis(4.0f) == doctest::Approx(6.0f));
        CHECK(spreadHysteresis(10.0f) == doctest::Approx(6.0f));
    }
}
