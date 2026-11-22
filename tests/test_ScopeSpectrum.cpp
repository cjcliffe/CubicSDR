// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/ScopeSpectrum.h"

TEST_SUITE("ScopeSpectrum") {
    TEST_CASE("equal rates keep every bin") {
        CHECK(scopeSpectrumOutSize(512, 44100, 44100) == 512);
    }

    TEST_CASE("lower output rate keeps the low bins") {
        CHECK(scopeSpectrumOutSize(512, 22050, 44100) == 256);
        CHECK(scopeSpectrumOutSize(512, 30000, 44100) == 348);
    }

    TEST_CASE("higher output rate never exceeds the bin count") {
        CHECK(scopeSpectrumOutSize(512, 44100, 22050) == 512);
        CHECK(scopeSpectrumOutSize(512, 48000, 8000) == 512);
    }

    TEST_CASE("degenerate rates stay in range") {
        CHECK(scopeSpectrumOutSize(512, 44100, 0) == 512);
        CHECK(scopeSpectrumOutSize(512, 1, 44100) == 1);
        CHECK(scopeSpectrumOutSize(0, 44100, 22050) == 0);
    }

    TEST_CASE("point x spans the full range") {
        CHECK(scopePointX(0, 57) == 0.0);
        CHECK(scopePointX(56, 57) == 1.0);
        CHECK(scopePointX(1, 3) == 0.5);
        CHECK(scopePointX(0, 1) == 0.0);
        CHECK(scopePointX(5, 1) == 0.0);
    }
}
