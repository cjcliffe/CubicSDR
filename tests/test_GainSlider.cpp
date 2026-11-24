// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/GainSlider.h"

using namespace GainSlider;

TEST_SUITE("GainSlider") {
    TEST_CASE("hit maps height to value") {
        CHECK(hitValue(0.0f, 0.0f, 100.0f) == 50.0f);
        CHECK(hitValue(-1.0f, 0.0f, 100.0f) == 0.0f);
        CHECK(hitValue(1.0f, 0.0f, 100.0f) == 100.0f);
        CHECK(hitValue(2.0f, 0.0f, 100.0f) == 100.0f);
        CHECK(hitValue(-2.0f, 0.0f, 100.0f) == 0.0f);
    }

    TEST_CASE("gain clamps to range") {
        CHECK(clampGain(5.0f, 0.0f, 10.0f) == 5.0f);
        CHECK(clampGain(15.0f, 0.0f, 10.0f) == 10.0f);
        CHECK(clampGain(-5.0f, 0.0f, 10.0f) == 0.0f);
    }

    TEST_CASE("label truncates toward zero") {
        CHECK(gainLabelText(5.9f) == "5");
        CHECK(gainLabelText(-5.9f) == "-5");
        CHECK(gainLabelText(0.0f) == "0");
    }

    TEST_CASE("level normalizes to range") {
        CHECK(levelNorm(5.0f, 0.0f, 10.0f) == 0.5f);
        CHECK(levelNorm(0.0f, 0.0f, 10.0f) == 0.0f);
        CHECK(levelNorm(10.0f, 0.0f, 10.0f) == 1.0f);
    }

    TEST_CASE("label layout reserves meter room") {
        LabelLayout layout = meterLabelLayout(400.0, 1.0, 18.0);
        CHECK(layout.bgH == doctest::Approx(0.915));
        CHECK(layout.valH == doctest::Approx(0.045));
        CHECK(layout.valY == doctest::Approx(-0.9575));
        CHECK(layout.labY == doctest::Approx(0.9575));
        CHECK(layout.labH == doctest::Approx(0.045));
    }

    TEST_CASE("strip lays out bars across width") {
        GainStrip strip = gainStripLayout(2);
        CHECK(strip.spacing == 1.0f);
        CHECK(strip.barWidth == doctest::Approx(0.35).epsilon(1e-5));
        CHECK(strip.startPos == 0.5f);
        CHECK(strip.barHeight == 1.0f);
        CHECK(gainBarMidX(strip, 0) == -0.5f);
        CHECK(gainBarMidX(strip, 1) == 0.5f);
        GainStrip single = gainStripLayout(1);
        CHECK(single.barWidth == doctest::Approx(0.7).epsilon(1e-5));
        CHECK(single.startPos == 1.0f);
    }

    TEST_CASE("wheel steps by rotation") {
        CHECK(wheelGainStep(50.0f, 120, 0.0f, 100.0f) == doctest::Approx(53.6).epsilon(1e-4));
        CHECK(wheelGainStep(50.0f, -120, 0.0f, 100.0f) == doctest::Approx(46.4).epsilon(1e-4));
        CHECK(wheelGainStep(50.0f, 0, 0.0f, 100.0f) == 50.0f);
    }

    TEST_CASE("refresh triggers on display change") {
        CHECK(roundGain(5.4) == 5.0f);
        CHECK(roundGain(5.5) == 6.0f);
        CHECK(!gainDisplayDiffers(5.0f, 5.9f));
        CHECK(gainDisplayDiffers(6.0f, 5.9f));
    }
}
