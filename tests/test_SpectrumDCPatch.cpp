// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "process/SpectrumDCPatch.h"

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
