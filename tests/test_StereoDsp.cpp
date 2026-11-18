// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "modules/modem/analog/StereoDsp.h"

#include <algorithm>
#include <cmath>
#include <vector>

TEST_SUITE("StereoDsp") {
    TEST_CASE("reference phase offsets the pilot filter phase by pi/4") {
        CHECK(stereoReferencePhase(0.0f) == doctest::Approx((float)(-M_PI / 4.0)));
        CHECK(stereoReferencePhase((float)M_PI) == doctest::Approx((float)(3.0 * M_PI / 4.0)));
    }

    TEST_CASE("delay line shifts an impulse by the delay") {
        MonoDelayLine line(22);
        std::vector<float> in(64, 0.0f), out(64, 0.0f);
        in[0] = 1.0f;
        line.apply(&in[0], in.size(), &out[0]);
        for (size_t i = 0; i < out.size(); i++) {
            CHECK(out[i] == doctest::Approx(i == 22 ? 1.0f : 0.0f));
        }
    }

    TEST_CASE("delay line carries across block boundaries") {
        MonoDelayLine line(22);
        std::vector<float> in(64, 0.0f), out(64, 0.0f);
        in[63] = 1.0f;
        line.apply(&in[0], in.size(), &out[0]);
        for (size_t i = 0; i < out.size(); i++) {
            CHECK(out[i] == doctest::Approx(0.0f));
        }
        std::fill(in.begin(), in.end(), 0.0f);
        line.apply(&in[0], in.size(), &out[0]);
        for (size_t i = 0; i < out.size(); i++) {
            CHECK(out[i] == doctest::Approx(i == 21 ? 1.0f : 0.0f));
        }
    }

    TEST_CASE("zero delay passes through") {
        MonoDelayLine line;
        CHECK(line.delay() == 0);
        std::vector<float> in(16, 0.0f), out(16, 0.0f);
        for (size_t i = 0; i < in.size(); i++) {
            in[i] = (float)i;
        }
        line.apply(&in[0], in.size(), &out[0]);
        for (size_t i = 0; i < out.size(); i++) {
            CHECK(out[i] == doctest::Approx((float)i));
        }
    }

    TEST_CASE("delay longer than a block still lands on stream time") {
        MonoDelayLine line(8);
        std::vector<float> in(4, 0.0f), out(4, 0.0f);
        in[0] = 1.0f;
        line.apply(&in[0], in.size(), &out[0]);
        for (size_t i = 0; i < out.size(); i++) {
            CHECK(out[i] == doctest::Approx(0.0f));
        }
        std::fill(in.begin(), in.end(), 0.0f);
        line.apply(&in[0], in.size(), &out[0]);
        for (size_t i = 0; i < out.size(); i++) {
            CHECK(out[i] == doctest::Approx(0.0f));
        }
        line.apply(&in[0], in.size(), &out[0]);
        for (size_t i = 0; i < out.size(); i++) {
            CHECK(out[i] == doctest::Approx(i == 0 ? 1.0f : 0.0f));
        }
    }

    TEST_CASE("blender starts mono and opens on a strong pilot") {
        StereoBlender blend(200000.0);
        CHECK(blend.process(0.0f, 0.0f) == doctest::Approx(0.0f));
        float g = 0.0f;
        for (int i = 0; i < 200000; i++) {
            g = blend.process(0.1f, 0.0f);
        }
        CHECK(g > 0.99f);
    }

    TEST_CASE("blender rests mid-scale on a mid-scale pilot") {
        StereoBlender blend(200000.0);
        float g = 0.0f;
        for (int i = 0; i < 200000; i++) {
            g = blend.process(0.0469f, 0.0f);
        }
        CHECK(g == doctest::Approx(0.5f).epsilon(0.1));
    }

    TEST_CASE("blender closes slower than it opens") {
        StereoBlender blend(200000.0);
        for (int i = 0; i < 200000; i++) {
            blend.process(0.1f, 0.0f);
        }
        float g = 1.0f;
        for (int i = 0; i < 20000; i++) {
            g = blend.process(0.0f, 0.0f);
        }
        CHECK(g > 0.5f);
        for (int i = 0; i < 400000; i++) {
            g = blend.process(0.0f, 0.0f);
        }
        CHECK(g < 0.01f);
    }

    TEST_CASE("blender reset drops the gain immediately") {
        StereoBlender blend(200000.0);
        for (int i = 0; i < 200000; i++) {
            blend.process(0.1f, 0.0f);
        }
        blend.reset();
        CHECK(blend.currentGain() == doctest::Approx(0.0f));
        CHECK(blend.process(0.0f, 0.0f) == doctest::Approx(0.0f));
    }
}
