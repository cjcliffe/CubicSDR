// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "visual/WaterfallPacer.h"

TEST_SUITE("WaterfallPacer") {
    TEST_CASE("fresh pacer is paused until a rate is set") {
        WaterfallPacer pacer;
        CHECK(pacer.getLinesPerSecond() == 0);

        WaterfallPacer::Plan plan = pacer.plan(1.0, 100);
        CHECK(plan.insert == 0);
        CHECK(plan.dropBefore == 0);
        CHECK(plan.dropAfter == 0);
    }

    TEST_CASE("steady pace inserts one line per line-period") {
        WaterfallPacer pacer;
        pacer.setLinesPerSecond(30);
        CHECK(pacer.getLinesPerSecond() == 30);

        for (int i = 0; i < 5; i++) {
            WaterfallPacer::Plan plan = pacer.plan(1.0 / 30.0, 5);
            CHECK(plan.insert == 1);
            CHECK(plan.dropBefore == 0);
            CHECK(plan.dropAfter == 0);
        }
    }

    TEST_CASE("insertions are capped per frame after a stall") {
        WaterfallPacer pacer;
        pacer.setLinesPerSecond(30);

        // One full second banked with a deep queue: 30 lines due, capped.
        // maxSteps = (30 + 19) / 20 + 1 = 3; window = 3 * 3 = 9 lines.
        WaterfallPacer::Plan plan = pacer.plan(1.0, 100);
        CHECK(plan.insert == 3);
        // Backlog beyond the short window is discarded, not fast-forwarded.
        CHECK(plan.dropBefore == 91);
    }

    TEST_CASE("banked credit is forgiven instead of bursting") {
        WaterfallPacer pacer;
        pacer.setLinesPerSecond(30);

        // Six lines remain past the cap and pre-drop; the half window
        // (9 / 2 = 4) keeps four, so two are trimmed.
        WaterfallPacer::Plan plan = pacer.plan(1.0, 100);
        CHECK(plan.dropAfter == 2);

        // Next frame resumes at pace on the newest lines.
        WaterfallPacer::Plan next = pacer.plan(1.0 / 30.0, 4);
        CHECK(next.insert == 1);
        CHECK(next.dropBefore == 0);
        CHECK(next.dropAfter == 0);
    }

    TEST_CASE("idle frames with an empty queue burst capped on refill") {
        WaterfallPacer pacer;
        pacer.setLinesPerSecond(30);

        WaterfallPacer::Plan idle = pacer.plan(1.0, 0);
        CHECK(idle.insert == 0);
        CHECK(idle.dropBefore == 0);
        CHECK(idle.dropAfter == 0);

        WaterfallPacer::Plan refill = pacer.plan(1.0 / 30.0, 5);
        CHECK(refill.insert == 3);
        CHECK(refill.dropBefore == 0);
    }

    TEST_CASE("cap and window scale with the requested rate") {
        WaterfallPacer pacer;
        pacer.setLinesPerSecond(120);

        // maxSteps = (120 + 19) / 20 + 1 = 7; window = 7 * 3 = 21.
        WaterfallPacer::Plan plan = pacer.plan(1.0, 200);
        CHECK(plan.insert == 7);
        CHECK(plan.dropBefore == 179);
        CHECK(plan.dropAfter == 4);
    }
}
