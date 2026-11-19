// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/RateGate.h"

TEST_SUITE("RateGate") {
    TEST_CASE("fires immediately on first call") {
        RateGate g;
        CHECK(g.shouldFire(1000, 250));
    }

    TEST_CASE("suppresses calls inside the window") {
        RateGate g;
        CHECK(g.shouldFire(1000, 250));
        CHECK(!g.shouldFire(1001, 250));
        CHECK(!g.shouldFire(1249, 250));
    }

    TEST_CASE("refires once the window elapses") {
        RateGate g;
        CHECK(g.shouldFire(1000, 250));
        CHECK(g.shouldFire(1250, 250));
        CHECK(!g.shouldFire(1251, 250));
    }

    TEST_CASE("zero interval always fires") {
        RateGate g;
        CHECK(g.shouldFire(1000, 0));
        CHECK(g.shouldFire(1000, 0));
    }

    TEST_CASE("stale gate refires") {
        RateGate g;
        CHECK(g.shouldFire(1000, 250));
        CHECK(g.shouldFire(100000, 250));
    }
}
