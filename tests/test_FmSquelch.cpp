// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "demod/SquelchState.h"

#include <cmath>
#include <vector>

TEST_SUITE("FmSquelch") {
    // 2nd-order HP sits about -24 dB at a quarter of cutoff.
    TEST_CASE("voice-band tone reads low") {
        FmNoiseMeter m;
        std::vector<float> v(4800);
        for (size_t i = 0; i < v.size(); i++) {
            v[i] = sinf(2.0f * (float)M_PI * 1000.0f * (float)i / 48000.0f);
        }
        CHECK(m.process(&v[0], v.size(), 48000.0f) < -20.0f);
    }

    TEST_CASE("high hiss reads high") {
        FmNoiseMeter m;
        std::vector<float> v(4800);
        for (size_t i = 0; i < v.size(); i++) {
            v[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        }
        CHECK(m.process(&v[0], v.size(), 48000.0f) > -6.0f);
    }

    TEST_CASE("settled dc reads floor") {
        FmNoiseMeter m;
        std::vector<float> v(4800, 1.0f);
        m.process(&v[0], v.size(), 48000.0f);
        m.process(&v[0], v.size(), 48000.0f);
        CHECK(m.process(&v[0], v.size(), 48000.0f) < -100.0f);
    }

    TEST_CASE("empty block holds last") {
        FmNoiseMeter m;
        std::vector<float> v(4800, 1.0f);
        float first = m.process(&v[0], v.size(), 48000.0f);
        CHECK(m.process(nullptr, 0, 48000.0f) == first);
    }

    TEST_CASE("reference seeds and attacks up") {
        NoiseReference r;
        CHECK(r.process(-50.0f, 0.008f, true) == doctest::Approx(0.0f));
        CHECK(r.process(-40.0f, 0.008f, true) == doctest::Approx(0.0f));
        CHECK(r.getReference() == doctest::Approx(-40.0f));
    }

    TEST_CASE("quiet step reads depth without decay") {
        NoiseReference r;
        r.process(-40.0f, 0.008f, true);
        CHECK(r.process(-55.0f, 0.008f, false) == doctest::Approx(15.0f));
        CHECK(r.process(-55.0f, 0.008f, false) == doctest::Approx(15.0f));
    }

    TEST_CASE("decay runs only when allowed") {
        NoiseReference r;
        r.process(-40.0f, 0.008f, false);
        CHECK(r.process(-55.0f, 1.0f, true) == doctest::Approx(13.0f));
    }

    TEST_CASE("decay clamps at current") {
        NoiseReference r;
        r.process(-40.0f, 0.008f, true);
        CHECK(r.process(-39.5f, 10.0f, true) == doctest::Approx(0.0f));
    }
}
