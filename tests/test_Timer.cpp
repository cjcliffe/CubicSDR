// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "Timer.h"

TEST_SUITE("Timer") {
    TEST_CASE("locked framerate advances a fixed step per update") {
        Timer t;
        t.start();
        t.lockFramerate(30.0f);
        CHECK(t.locked());

        t.update();
        CHECK(t.getNumUpdates() == 1);
        CHECK(t.getMilliseconds() == 33);
        CHECK(t.lastUpdateMilliseconds() == 33);

        t.update();
        CHECK(t.getNumUpdates() == 2);
        CHECK(t.getMilliseconds() == 66);
    }

    TEST_CASE("pause freezes elapsed time but not total time") {
        Timer t;
        t.start();
        t.lockFramerate(30.0f);
        t.update();
        t.update();

        t.paused(true);
        CHECK(t.paused());
        t.update();
        CHECK(t.getMilliseconds() == 66);
        CHECK(t.totalMilliseconds() == 99);
    }

    TEST_CASE("setters take effect on the next update") {
        Timer t;
        t.start();
        t.lockFramerate(30.0f);
        t.update();

        t.setMilliseconds(1000);
        t.update();
        // The new base appears plus one locked step of 33ms.
        CHECK(t.getMilliseconds() == 1033);

        t.setSeconds(2.5);
        t.update();
        CHECK(t.getMilliseconds() == 2533);
        CHECK(t.getSeconds() == doctest::Approx(2.533));
    }
}
