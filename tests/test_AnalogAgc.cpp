// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "modules/modem/AnalogAgc.h"

#include <algorithm>
#include <cmath>
#include <vector>

static const float PI = 3.141592653589793f;

TEST_SUITE("AnalogAgc") {
    static std::vector<float> tone(float peak, size_t n) {
        std::vector<float> v(n);
        for (size_t i = 0; i < n; i++) {
            v[i] = peak * sinf(2.0f * PI * (float)i / 16.0f);
        }
        return v;
    }

    TEST_CASE("steady tone converges to target over peak") {
        AnalogAgc agc(5400.0);
        auto v = tone(0.25f, 512);
        float g = 0.0f;
        for (int b = 0; b < 200; b++) {
            g = agc.processBlock(&v[0], v.size());
        }
        CHECK(g == doctest::Approx(2.0f).epsilon(0.02));
    }

    TEST_CASE("nominal level is preserved") {
        AnalogAgc agc(5400.0);
        auto v = tone(1.0f, 512);
        float g = 0.0f;
        for (int b = 0; b < 200; b++) {
            g = agc.processBlock(&v[0], v.size());
        }
        CHECK(g == doctest::Approx(0.5f).epsilon(0.02));
    }

    TEST_CASE("attack settles a loud onset within one block") {
        AnalogAgc agc(5400.0);
        auto quiet = tone(0.01f, 512);
        for (int b = 0; b < 200; b++) {
            agc.processBlock(&quiet[0], quiet.size());
        }
        auto loud = tone(1.0f, 512);
        float g = agc.processBlock(&loud[0], loud.size());
        CHECK(g < 0.6f);
    }

    TEST_CASE("release moves slower than attack") {
        AnalogAgc agc(5400.0);
        auto loud = tone(1.0f, 512);
        for (int b = 0; b < 200; b++) {
            agc.processBlock(&loud[0], loud.size());
        }
        auto quiet = tone(0.01f, 512);
        float g = agc.processBlock(&quiet[0], quiet.size());
        CHECK(g > 0.5f);
        CHECK(g < 1.0f);
    }

    TEST_CASE("silence rails at the ceiling, never infinity") {
        AnalogAgc agc(5400.0);
        std::vector<float> v(512, 0.0f);
        float g = 0.0f;
        for (int b = 0; b < 500; b++) {
            g = agc.processBlock(&v[0], v.size());
        }
        CHECK(g == doctest::Approx(1000.0f));
        CHECK(std::isfinite(g));
    }

    TEST_CASE("weak signals reach target below the ceiling") {
        AnalogAgc agc(5400.0);
        auto v = tone(0.001f, 512);
        float g = 0.0f;
        for (int b = 0; b < 500; b++) {
            g = agc.processBlock(&v[0], v.size());
        }
        CHECK(g == doctest::Approx(500.0f).epsilon(0.02));
    }

    TEST_CASE("negative peaks count the same as positive") {
        AnalogAgc agc(5400.0);
        std::vector<float> v(512, -0.7f);
        float g = 0.0f;
        for (int b = 0; b < 200; b++) {
            g = agc.processBlock(&v[0], v.size());
        }
        CHECK(g == doctest::Approx(0.5f / 0.7f).epsilon(0.02));
    }

    TEST_CASE("empty block leaves gain alone") {
        AnalogAgc agc(5400.0);
        CHECK(agc.processBlock(nullptr, 0) == doctest::Approx(0.5f));
    }

    TEST_CASE("reset restores unity level") {
        AnalogAgc agc(5400.0);
        auto v = tone(0.1f, 512);
        for (int b = 0; b < 200; b++) {
            agc.processBlock(&v[0], v.size());
        }
        agc.reset();
        CHECK(agc.currentGain() == doctest::Approx(0.5f));
    }

    TEST_CASE("processLevel slews toward the given peak") {
        AnalogAgc agc(5400.0);
        CHECK(agc.processLevel(0.25f, 512) == doctest::Approx(0.6276f).epsilon(0.005));
    }
}

TEST_SUITE("DcBlocker") {
    TEST_CASE("dc step decays to zero") {
        DcBlocker dc;
        float y = 0.0f;
        for (int i = 0; i < 20000; i++) {
            y = dc.process(1.0f);
        }
        CHECK(fabsf(y) < 1e-3f);
    }

    TEST_CASE("audio tone passes untouched") {
        DcBlocker dc;
        double rms = 0.0;
        const int n = 5400;
        for (int i = 0; i < n; i++) {
            float x = sinf(2.0f * PI * 1000.0f * (float)i / 5400.0f);
            float y = dc.process(x);
            if (i >= n / 2) {
                rms += (double)(y * y);
            }
        }
        rms = sqrt(rms / (n / 2));
        CHECK(rms == doctest::Approx(1.0 / sqrt(2.0)).epsilon(0.01));
    }

    TEST_CASE("reset clears state") {
        DcBlocker dc;
        for (int i = 0; i < 100; i++) {
            dc.process(1.0f);
        }
        dc.reset();
        CHECK(dc.process(0.0f) == doctest::Approx(0.0f));
    }
}

TEST_SUITE("GainDb") {
    TEST_CASE("round trip") {
        CHECK(dbToLinearGain(linearGainToDb(31.6227766f)) == doctest::Approx(31.6227766f).epsilon(1e-5));
        CHECK(dbToLinearGain(0.0f) == doctest::Approx(1.0f));
        CHECK(linearGainToDb(1.0f) == doctest::Approx(0.0f));
    }

    TEST_CASE("twenty db is ten times amplitude") {
        CHECK(dbToLinearGain(20.0f) == doctest::Approx(10.0f));
        CHECK(dbToLinearGain(40.0f) == doctest::Approx(100.0f));
        CHECK(linearGainToDb(100.0f) == doctest::Approx(40.0f));
    }

    TEST_CASE("zero gain floors instead of inf") {
        CHECK(std::isfinite(linearGainToDb(0.0f)));
    }
}

TEST_SUITE("ToneLevel") {
    static std::vector<float> wav(float freq, float peak, size_t n, double fs = 48000.0) {
        std::vector<float> v(n);
        for (size_t i = 0; i < n; i++) {
            v[i] = peak * sinf(2.0f * PI * freq * (float)i / (float)fs);
        }
        return v;
    }

    TEST_CASE("tone reads its peak amplitude") {
        ToneLevelDetector det(48000.0, 650.0);
        auto v = wav(650.0f, 1.0f, 4800);
        CHECK(det.measureBlock(&v[0], v.size()) == doctest::Approx(1.0f).epsilon(0.02));
    }

    TEST_CASE("off-tone reads far lower") {
        ToneLevelDetector det(48000.0, 650.0);
        auto on = wav(650.0f, 1.0f, 4800);
        auto off = wav(2000.0f, 1.0f, 4800);
        float onLevel = det.measureBlock(&on[0], on.size());
        det.reset();
        float offLevel = det.measureBlock(&off[0], off.size());
        CHECK(onLevel > 20.0f * offLevel);
    }

    TEST_CASE("impulses barely move the reading") {
        ToneLevelDetector det(48000.0, 650.0);
        auto v = wav(650.0f, 1.0f, 4800);
        for (size_t i = 0; i < v.size(); i += 1000) {
            v[i] += 5.0f;
        }
        CHECK(det.measureBlock(&v[0], v.size()) == doctest::Approx(1.0f).epsilon(0.05));
    }

    TEST_CASE("retune follows the beep") {
        ToneLevelDetector det(48000.0, 650.0);
        auto v = wav(1200.0f, 1.0f, 4800);
        float before = det.measureBlock(&v[0], v.size());
        det.configure(48000.0, 1200.0);
        det.reset();
        float after = det.measureBlock(&v[0], v.size());
        CHECK(after == doctest::Approx(1.0f).epsilon(0.02));
        CHECK(after > 5.0f * before);
    }
}
