// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "process/SpectrumFftSize.h"

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
