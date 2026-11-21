// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "process/SpectrumDsp.h"

#include <cmath>

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

TEST_SUITE("SpectrumDecimate") {
    TEST_CASE("peak wins over span") {
        std::vector<double> bins(64, 1.0);
        bins[30] = 9.0;
        std::vector<unsigned int> idxs;
        for (unsigned int i = 20; i < 40; i++) {
            idxs.push_back(i);
        }
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(9.0));
    }

    TEST_CASE("single bin passes through") {
        std::vector<double> bins(64, 1.0);
        bins[25] = 4.0;
        CHECK(reducePixelMax(bins, {25}, 0.5) == doctest::Approx(4.0));
    }

    TEST_CASE("out-of-range indexes read the floor") {
        std::vector<double> bins(64, 1.0);
        std::vector<unsigned int> idxs{64, 100, 4000000000u};
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(0.5));
    }

    TEST_CASE("bin zero is excluded") {
        std::vector<double> bins(64, 1.0);
        bins[0] = 50.0;
        std::vector<unsigned int> idxs{0, 10};
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(1.0));
    }

    TEST_CASE("empty span yields floor") {
        std::vector<double> bins(64, 1.0);
        std::vector<unsigned int> idxs;
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(0.5));
    }

    TEST_CASE("all below floor yields floor") {
        std::vector<double> bins(64, 0.1);
        std::vector<unsigned int> idxs{10, 20, 30};
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(0.5));
    }
}

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

TEST_SUITE("SpectrumFftSize") {
    TEST_CASE("full size above channelizer range") {
        CHECK(effectiveSpectrumFFT(2048, 500001) == 2048);
        CHECK(effectiveSpectrumFFT(8192, 2000000) == 8192);
    }

    TEST_CASE("half size within channelizer range") {
        CHECK(effectiveSpectrumFFT(2048, 500000) == 1024);
        CHECK(effectiveSpectrumFFT(2048, 62501) == 1024);
    }

    TEST_CASE("quarter size at low rates") {
        CHECK(effectiveSpectrumFFT(2048, 62500) == 512);
        CHECK(effectiveSpectrumFFT(2048, 1000) == 512);
    }

    TEST_CASE("scales from configured base") {
        CHECK(effectiveSpectrumFFT(4096, 1000000) == 4096);
        CHECK(effectiveSpectrumFFT(4096, 250000) == 2048);
        CHECK(effectiveSpectrumFFT(4096, 30000) == 1024);
    }

    TEST_CASE("low size for constrained devices") {
        CHECK(effectiveSpectrumFFT(1024, 1000000) == 1024);
        CHECK(effectiveSpectrumFFT(1024, 250000) == 512);
        CHECK(effectiveSpectrumFFT(1024, 30000) == 256);
    }
}

TEST_SUITE("SpectrumDCPatch") {
    TEST_CASE("centers on DC at base size") {
        DCRange r = dcPatchRange(1200000, 2400000, 2048);
        CHECK(r.start == 1023);
        CHECK(r.end == 1025);
    }

    TEST_CASE("centers on DC at larger sizes") {
        DCRange r = dcPatchRange(1200000, 2400000, 4096);
        CHECK(r.start == 2045);
        CHECK(r.end == 2051);
        r = dcPatchRange(1200000, 2400000, 8192);
        CHECK(r.start == 4090);
        CHECK(r.end == 4102);
    }

    TEST_CASE("tracks off-center DC") {
        DCRange r = dcPatchRange(600000, 2400000, 2048);
        CHECK(r.start == 511);
        CHECK(r.end == 513);
    }

    TEST_CASE("keeps minimum two-point width") {
        DCRange r = dcPatchRange(5000000, 10000000, 2048);
        CHECK(r.end - r.start == 2);
    }

    TEST_CASE("invalid range yields empty") {
        DCRange r = dcPatchRange(0, 0, 2048);
        CHECK(r.start == 0);
        CHECK(r.end == 0);
    }
}
