// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "sdr/TuneRange.h"

TEST_SUITE("TuneRange") {
    TEST_CASE("in-range frequencies pass through") {
        CHECK(clampTuneFrequency(100000000LL, 0, 24000000LL, 1766000000LL) == 100000000LL);
        CHECK(clampTuneFrequency(24000000LL, 0, 24000000LL, 1766000000LL) == 24000000LL);
        CHECK(clampTuneFrequency(1766000000LL, 0, 24000000LL, 1766000000LL) == 1766000000LL);
    }

    TEST_CASE("out-of-range frequencies clamp to the device window") {
        CHECK(clampTuneFrequency(10000000LL, 0, 24000000LL, 1766000000LL) == 24000000LL);
        CHECK(clampTuneFrequency(2000000000LL, 0, 24000000LL, 1766000000LL) == 1766000000LL);
    }

    TEST_CASE("tune floor never exceeds the legacy floor") {
        CHECK(deviceTuneFloor(24000000LL, 2400000L) == 1200000LL);
        CHECK(deviceTuneFloor(1200000LL, 2400000L) == 1200000LL);
    }

    TEST_CASE("tune floor drops when the driver opens room") {
        CHECK(deviceTuneFloor(0LL, 2400000L) == 0LL);
        CHECK(deviceTuneFloor(500000LL, 2400000L) == 500000LL);
    }

    TEST_CASE("clamp applies to the offset-adjusted driver frequency") {
        CHECK(clampTuneFrequency(150000000LL, 125000000LL, 24000000LL, 1766000000LL) == 150000000LL);
        CHECK(clampTuneFrequency(140000000LL, 125000000LL, 24000000LL, 1766000000LL) == 149000000LL);
    }

    TEST_CASE("centered views pass through") {
        CHECK(pinViewCenter(100000000LL, 1000000LL, 100000000LL, 2000000L) == 100000000LL);
    }

    TEST_CASE("views past the data edge pin to the window") {
        CHECK(pinViewCenter(99000000LL, 1000000LL, 100000000LL, 2000000L) == 99500000LL);
        CHECK(pinViewCenter(101000000LL, 1000000LL, 100000000LL, 2000000L) == 100500000LL);
    }

    TEST_CASE("views wider than the data collapse to device center") {
        CHECK(pinViewCenter(99000000LL, 4000000LL, 100000000LL, 2000000L) == 100000000LL);
        CHECK(pinViewCenter(99000000LL, 2000000LL, 100000000LL, 2000000L) == 100000000LL);
    }

    TEST_CASE("demod centers inside the view pass through") {
        CHECK(pinDemodCenter(100000000LL, 100000000LL, 2000000LL) == 100000000LL);
    }

    TEST_CASE("demod centers past the view edge pin to it") {
        CHECK(pinDemodCenter(98000000LL, 100000000LL, 2000000LL) == 99000000LL);
        CHECK(pinDemodCenter(102000000LL, 100000000LL, 2000000LL) == 101000000LL);
    }

    TEST_CASE("sub-zero views inside the data window are allowed") {
        CHECK(pinViewCenter(100000LL, 1000000LL, 500000LL, 2400000L) == 100000LL);
        CHECK(pinViewCenter(-500000LL, 1000000LL, 500000LL, 2400000L) == -200000LL);
    }

    TEST_CASE("delta-lock offset is frequency minus center") {
        CHECK(deltaLockOffset(101000000LL, 100000000LL) == 1000000L);
        CHECK(deltaLockOffset(99000000LL, 100000000LL) == -1000000L);
        CHECK(deltaLockOffset(100000000LL, 100000000LL) == 0L);
    }

    TEST_CASE("delta-lock offset round-trips through the post-thread enforcement") {
        CHECK(100000000LL + deltaLockOffset(101000000LL, 100000000LL) == 101000000LL);
        CHECK(100000000LL + deltaLockOffset(99000000LL, 100000000LL) == 99000000LL);
    }
}
