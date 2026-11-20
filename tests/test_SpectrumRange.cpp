// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "process/SpectrumRange.h"

TEST_SUITE("SpectrumRange") {
    TEST_CASE("finds min and max without exclusion") {
        std::vector<double> bins(64, 1.0);
        bins[10] = 5.0;
        bins[20] = 0.25;
        SpectrumRange r = scanSpectrumRange(bins, false, 32);
        CHECK(r.ceil == doctest::Approx(5.0));
        CHECK(r.floor == doctest::Approx(0.25));
    }

    TEST_CASE("DC spike sets ceiling when shown") {
        std::vector<double> bins(64, 1.0);
        bins[32] = 40.0;
        SpectrumRange r = scanSpectrumRange(bins, false, 32);
        CHECK(r.ceil == doctest::Approx(40.0));
    }

    TEST_CASE("DC spike excluded when hidden") {
        std::vector<double> bins(64, 1.0);
        bins[32] = 40.0;
        bins[10] = 3.0;
        SpectrumRange r = scanSpectrumRange(bins, true, 32);
        CHECK(r.ceil == doctest::Approx(3.0));
    }

    TEST_CASE("bins outside the window still count") {
        std::vector<double> bins(64, 1.0);
        bins[32 - SPECTRUM_DC_EXCLUDE_HALF_WIDTH - 1] = 7.0;
        bins[32 + SPECTRUM_DC_EXCLUDE_HALF_WIDTH + 1] = 0.1;
        SpectrumRange r = scanSpectrumRange(bins, true, 32);
        CHECK(r.ceil == doctest::Approx(7.0));
        CHECK(r.floor == doctest::Approx(0.1));
    }

    TEST_CASE("empty input keeps initial range") {
        std::vector<double> bins;
        SpectrumRange r = scanSpectrumRange(bins, true, 0);
        CHECK(r.ceil == doctest::Approx(0.0));
        CHECK(r.floor == doctest::Approx(1.0));
    }
}
