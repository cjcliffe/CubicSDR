// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "process/SpectrumWindow.h"

TEST_SUITE("SpectrumWindow") {
    TEST_CASE("endpoints taper to zero") {
        std::vector<float> taps;
        generateHannWindow(taps, 64);
        CHECK(taps.size() == 64);
        CHECK(taps[0] == doctest::Approx(0.0f));
        CHECK(taps[63] == doctest::Approx(0.0f));
    }

    TEST_CASE("coherent gain matches unwindowed level") {
        std::vector<float> taps;
        generateHannWindow(taps, 1024);
        double sum = 0.0;
        for (float t : taps) {
            sum += t;
        }
        CHECK(sum / 1024.0 == doctest::Approx(1023.0 / 1024.0));
    }

    TEST_CASE("symmetric lobe") {
        std::vector<float> taps;
        generateHannWindow(taps, 64);
        for (size_t i = 0; i < 64; i++) {
            CHECK(taps[i] == doctest::Approx(taps[63 - i]));
        }
        CHECK(taps[32] > taps[0]);
    }

    TEST_CASE("degenerate sizes stay neutral") {
        std::vector<float> taps;
        generateHannWindow(taps, 0);
        CHECK(taps.empty());
        generateHannWindow(taps, 1);
        CHECK(taps.size() == 1);
        CHECK(taps[0] == doctest::Approx(1.0f));
    }
}
