// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "demod/DemodCycle.h"

namespace {

TEST_SUITE("DemodCycle") {
    TEST_CASE("empty list selects nothing") {
        CHECK(DemodCycle::step(0, -1, true) == -1);
        CHECK(DemodCycle::step(0, -1, false) == -1);
        CHECK(DemodCycle::step(0, 0, true) == -1);
    }

    TEST_CASE("forward steps and wraps to front") {
        CHECK(DemodCycle::step(3, 0, true) == 1);
        CHECK(DemodCycle::step(3, 1, true) == 2);
        CHECK(DemodCycle::step(3, 2, true) == 0);
    }

    TEST_CASE("backward steps and wraps to back") {
        CHECK(DemodCycle::step(3, 2, false) == 1);
        CHECK(DemodCycle::step(3, 1, false) == 0);
        CHECK(DemodCycle::step(3, 0, false) == 2);
    }

    TEST_CASE("single entry stays put") {
        CHECK(DemodCycle::step(1, 0, true) == 0);
        CHECK(DemodCycle::step(1, 0, false) == 0);
    }

    TEST_CASE("no selection enters at first in view") {
        CHECK(DemodCycle::step(3, -1, true) == 0);
    }

    TEST_CASE("no selection enters at last in view going backward") {
        CHECK(DemodCycle::step(3, -1, false) == 2);
    }

    TEST_CASE("stale selection re-enters instead of stalling") {
        CHECK(DemodCycle::step(3, 7, true) == 0);
        CHECK(DemodCycle::step(3, 7, false) == 2);
    }
}

} // namespace
