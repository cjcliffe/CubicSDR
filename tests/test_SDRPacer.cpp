// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "sdr/SDRPacer.h"

TEST_SUITE("SDRPacer") {
    TEST_CASE("fresh pacer emits nothing on an empty queue") {
        SDRPacer pacer;
        CHECK(pacer.getCapacity() == 64);
        CHECK(!pacer.isBypassed());

        SDRPacer::Plan plan = pacer.plan(1.0, 0);
        CHECK(plan.emit == 0);
        CHECK(plan.drop == 0);
        CHECK(plan.waitSeconds == 0.0);
    }

    TEST_CASE("steady pace emits one frame per period") {
        SDRPacer pacer;

        for (int i = 0; i < 5; i++) {
            SDRPacer::Plan plan = pacer.plan(1.0 / 60.0, 5);
            CHECK(plan.emit == 1);
            CHECK(plan.drop == 0);
            CHECK(plan.waitSeconds == 0.0);
        }
    }

    TEST_CASE("banked credit drains at most the burst bound per tick") {
        SDRPacer pacer;

        // A full second banked: 60 frames due, but only the burst bound
        // goes out per tick so a stall drains gradually instead of
        // flooding downstream.
        SDRPacer::Plan plan = pacer.plan(1.0, 31);
        CHECK(plan.emit == 4);
        CHECK(plan.drop == 0);
        CHECK(plan.waitSeconds == 0.0);

        // Next period resumes at pace.
        SDRPacer::Plan next = pacer.plan(1.0 / 60.0, 27);
        CHECK(next.emit == 1);
        CHECK(next.drop == 0);
    }

    TEST_CASE("systematic overshoot keeps pace instead of ratcheting down") {
        SDRPacer pacer;

        // Each tick earns just over two frames, as late ticks do under
        // timer overshoot. Earnings must be honored, not forgiven, or
        // output falls permanently behind a 60Hz input.
        int total = 0;
        for (int i = 0; i < 60; i++) {
            SDRPacer::Plan plan = pacer.plan(2.07 / 60.0, 64);
            CHECK(plan.waitSeconds == 0.0);
            total += plan.emit;
        }
        CHECK(total == 124);
    }

    TEST_CASE("backlog beyond capacity is discarded oldest-first") {
        SDRPacer pacer;

        SDRPacer::Plan plan = pacer.plan(1.0 / 60.0, 100);
        CHECK(plan.emit == 1);
        CHECK(plan.drop == 36);
    }

    TEST_CASE("dry queue waits on input") {
        SDRPacer pacer;

        SDRPacer::Plan plan = pacer.plan(1.0 / 60.0, 0);
        CHECK(plan.emit == 0);
        CHECK(plan.drop == 0);
        CHECK(plan.waitSeconds == 0.0);
    }

    TEST_CASE("wait hint covers the remainder of the period") {
        SDRPacer pacer;

        SDRPacer::Plan plan = pacer.plan(1.0 / 120.0, 5);
        CHECK(plan.emit == 0);
        CHECK(plan.drop == 0);
        CHECK(plan.waitSeconds == doctest::Approx(1.0 / 120.0));
    }

    TEST_CASE("non-positive dt does not advance credit") {
        SDRPacer pacer;

        SDRPacer::Plan stalled = pacer.plan(0.0, 5);
        CHECK(stalled.emit == 0);
        CHECK(stalled.waitSeconds == doctest::Approx(1.0 / 60.0));

        SDRPacer::Plan backwards = pacer.plan(-1.0, 5);
        CHECK(backwards.emit == 0);
        CHECK(backwards.waitSeconds == doctest::Approx(1.0 / 60.0));
    }

    TEST_CASE("capacity clamps to at least one frame") {
        SDRPacer pacer;
        pacer.setCapacity(0);
        CHECK(pacer.getCapacity() == 1);
        pacer.setCapacity(-5);
        CHECK(pacer.getCapacity() == 1);
        pacer.setCapacity(4);
        CHECK(pacer.getCapacity() == 4);

        SDRPacer::Plan plan = pacer.plan(0.0, 10);
        CHECK(plan.emit == 0);
        CHECK(plan.drop == 6);
    }

    TEST_CASE("stream tracking adopts then reports discontinuities") {
        SDRPacer pacer;

        CHECK(!pacer.noteBlock(2048000, 4));
        CHECK(!pacer.noteBlock(2048000, 4));
        CHECK(pacer.noteBlock(1024000, 4));
        CHECK(pacer.noteBlock(2048000, 2));

        pacer.reset();
        CHECK(!pacer.noteBlock(1024000, 4));
        CHECK(pacer.noteBlock(1024000, 1));
    }

    TEST_CASE("reset clears banked credit") {
        SDRPacer pacer;
        pacer.plan(1.0, 5);

        pacer.reset();

        SDRPacer::Plan plan = pacer.plan(1.0 / 120.0, 5);
        CHECK(plan.emit == 0);
        CHECK(plan.waitSeconds == doctest::Approx(1.0 / 120.0));
    }

    TEST_CASE("bypass forwards everything immediately") {
        SDRPacer pacer;
        pacer.setBypass(true);
        CHECK(pacer.isBypassed());

        SDRPacer::Plan plan = pacer.plan(0.0, 10);
        CHECK(plan.emit == 10);
        CHECK(plan.drop == 0);
        CHECK(plan.waitSeconds == 0.0);

        // The cap still holds under overload while bypassed.
        pacer.setCapacity(4);
        SDRPacer::Plan capped = pacer.plan(0.0, 10);
        CHECK(capped.emit == 4);
        CHECK(capped.drop == 6);
    }
}
