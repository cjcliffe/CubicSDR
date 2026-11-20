// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "process/SpectrumAverage.h"

#include <cmath>

TEST_SUITE("SpectrumAverage") {
    TEST_CASE("decibel conversion is exact") {
        CHECK(magToDb(1.0) == doctest::Approx(0.0));
        CHECK(magToDb(10.0) == doctest::Approx(20.0));
        CHECK(dbToMag(20.0) == doctest::Approx(10.0));
        CHECK(dbToMag(0.0) == doctest::Approx(1.0));
    }

    TEST_CASE("zero and negative clamp to floor") {
        CHECK(magToDb(0.0) == doctest::Approx(-240.0));
        CHECK(magToDb(-5.0) == doctest::Approx(-240.0));
    }

    TEST_CASE("roundtrip preserves level") {
        const double mags[] = {0.001, 1.0, 1000.0};
        for (double mag : mags) {
            CHECK(dbToMag(magToDb(mag)) == doctest::Approx(mag));
        }
    }

    TEST_CASE("iir converges to steady input") {
        double state = 0.0;
        for (int i = 0; i < 20; i++) {
            state = iirStep(state, 10.0, 0.5);
        }
        CHECK(state == doctest::Approx(10.0));
    }

    TEST_CASE("iir rate bounds") {
        CHECK(iirStep(3.0, 10.0, 1.0) == doctest::Approx(10.0));
        CHECK(iirStep(3.0, 10.0, 0.0) == doctest::Approx(3.0));
    }

    TEST_CASE("two-stage smooths alternating input") {
        double ma = 0.0, maa = 0.0;
        for (int i = 0; i < 200; i++) {
            double db = (i % 2 == 0) ? 0.0 : 100.0;
            ma = iirStep(ma, db, 0.5);
            maa = iirStep(maa, ma, 0.5);
        }
        CHECK(maa == doctest::Approx(500.0 / 9.0));
        ma = iirStep(ma, 0.0, 0.5);
        maa = iirStep(maa, ma, 0.5);
        CHECK(maa == doctest::Approx(400.0 / 9.0));
    }
}
