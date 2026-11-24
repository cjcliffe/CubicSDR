// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/ModeSelector.h"

using namespace ModeSelector;

TEST_SUITE("ModeSelector") {
    TEST_CASE("hovered row maps mouse height to choice") {
        CHECK(hoveredRow(0.5f, 4, true) == 2);
        CHECK(hoveredRow(0.25f, 4, true) == 3);
        CHECK(hoveredRow(0.75f, 4, true) == 1);
        CHECK(hoveredRow(0.1f, 4, true) == 3);
    }

    TEST_CASE("hovered row clamps edges and misses") {
        CHECK(hoveredRow(0.0f, 4, true) == 3);
        CHECK(hoveredRow(1.0f, 4, true) == 0);
        CHECK(hoveredRow(0.5f, 4, false) == -1);
        CHECK(hoveredRow(0.5f, 0, true) == -1);
    }

    TEST_CASE("release selects clicked choice") {
        ReleaseResult picked = resolveRelease(0, 2, false, true, false);
        CHECK(picked.selection == 2);
        CHECK(picked.changed);
        ReleaseResult reclick = resolveRelease(2, 2, false, true, false);
        CHECK(reclick.selection == 2);
        CHECK(reclick.changed);
        ReleaseResult drag = resolveRelease(2, 0, false, false, false);
        CHECK(drag.selection == 2);
        CHECK(!drag.changed);
    }

    TEST_CASE("release toggles off on reselect") {
        ReleaseResult off = resolveRelease(2, 2, true, true, false);
        CHECK(off.selection == -1);
        CHECK(off.changed);
        ReleaseResult switchChoice = resolveRelease(2, 1, true, true, false);
        CHECK(switchChoice.selection == 1);
        CHECK(switchChoice.changed);
        ReleaseResult stillOff = resolveRelease(-1, 0, true, false, false);
        CHECK(stillOff.selection == -1);
        CHECK(!stillOff.changed);
    }

    TEST_CASE("release ignores disabled control") {
        ReleaseResult held = resolveRelease(2, 0, false, true, true);
        CHECK(held.selection == 2);
        CHECK(!held.changed);
    }

    TEST_CASE("pulse phase follows wall clock") {
        CHECK(pulsePhaseAt(0) == 0.0f);
        CHECK(pulsePhaseAt(500) == doctest::Approx(1.5707963).epsilon(1e-5));
        CHECK(pulsePhaseAt(1000) == doctest::Approx(3.1415927).epsilon(1e-5));
        CHECK(pulsePhaseAt(2000) == 0.0f);
        CHECK(pulsePhaseAt(2500) == doctest::Approx(1.5707963).epsilon(1e-5));
    }

    TEST_CASE("pulse blend breathes between off and on") {
        CHECK(pulseBlend(0.0f) == doctest::Approx(0.5));
        CHECK(pulseBlend(1.5707963f) == doctest::Approx(1.0));
        CHECK(pulseBlend(3.1415927f) == doctest::Approx(0.5));
        CHECK(pulseBlend(4.7123890f) == doctest::Approx(0.0));
    }

    TEST_CASE("selector cell lays out button geometry") {
        SelectorCell cell = selectorCell(0, 4, 100.0f, 400.0f, 4.0f, 4.0f);
        CHECK(cell.fontSize == 18);
        CHECK(cell.labelY == 0.75f);
        CHECK(cell.outline.size() == 8);
        CHECK(cell.outline[0] == doctest::Approx(-0.96).epsilon(1e-5));
        CHECK(cell.outline[2] == doctest::Approx(0.96).epsilon(1e-5));
        CHECK(cell.outline[1] == doctest::Approx(0.51).epsilon(1e-5));
        CHECK(cell.outline[5] == doctest::Approx(0.99).epsilon(1e-5));
    }

    TEST_CASE("selector cell shrinks font on small views") {
        CHECK(selectorCell(0, 4, 29.0f, 400.0f, 4.0f, 4.0f).fontSize == 16);
        CHECK(selectorCell(0, 4, 100.0f, 199.0f, 4.0f, 4.0f).fontSize == 16);
        CHECK(selectorCell(0, 4, 30.0f, 200.0f, 4.0f, 4.0f).fontSize == 18);
    }
}
