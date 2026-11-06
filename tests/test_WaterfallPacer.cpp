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
        // Ceiling is twice the steady cap of 3, so 6; window is 18 lines.
        WaterfallPacer::Plan plan = pacer.plan(1.0, 100);
        CHECK(plan.insert == 6);
        // Backlog beyond the short window is discarded, not fast-forwarded.
        CHECK(plan.dropBefore == 82);
    }

    TEST_CASE("banked credit is forgiven instead of bursting") {
        WaterfallPacer pacer;
        pacer.setLinesPerSecond(30);

        // Twelve lines remain past the cap and pre-drop; the half window
        // (18 / 2 = 9) keeps nine, so three are trimmed.
        WaterfallPacer::Plan plan = pacer.plan(1.0, 100);
        CHECK(plan.dropAfter == 3);

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

        // Ceiling is twice the steady cap of 7, so 14; window is 42 lines.
        WaterfallPacer::Plan plan = pacer.plan(1.0, 200);
        CHECK(plan.insert == 14);
        CHECK(plan.dropBefore == 158);
        CHECK(plan.dropAfter == 7);
    }

    TEST_CASE("requested rate is honored at low frame rates") {
        WaterfallPacer pacer;
        pacer.setLinesPerSecond(1024);

        // Fifteen 15fps frames with a deep queue: about 1024 lines owed,
        // and the time-scaled cap keeps up with them.
        int total = 0;
        for (int i = 0; i < 15; i++) {
            WaterfallPacer::Plan plan = pacer.plan(1.0 / 15.0, 500);
            total += plan.insert;
        }
        CHECK(total > 1000);
        CHECK(total <= 1030);
    }

    TEST_CASE("hitch does not repay as a burst") {
        WaterfallPacer pacer;
        pacer.setLinesPerSecond(1024);

        // Five stalled seconds would owe thousands of lines; the ceiling
        // (twice the steady cap of 53) holds the frame to 106 instead.
        WaterfallPacer::Plan plan = pacer.plan(5.0, 500);
        CHECK(plan.insert == 106);
        CHECK(plan.dropBefore == 182);
    }
}
