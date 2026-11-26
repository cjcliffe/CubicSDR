// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "core/CoreState.h"

TEST_SUITE("CoreState") {
    TEST_CASE("defaults match application startup") {
        CoreStateImpl state(2500000);
        CHECK(state.getFrequency() == 0);
        CHECK(state.getSampleRate() == 2500000);
        CHECK(state.getPPM() == 0);
        CHECK(state.getOffset() == 0);
        CHECK(state.getFrequencySnap() == 1);
        long long center = 1234;
        long long bandwidth = 5678;
        CHECK(state.getMainZoomedView(center, bandwidth) == false);
        CHECK(center == 1234);
        CHECK(bandwidth == 5678);
    }

    TEST_CASE("scalars round-trip") {
        CoreStateImpl state(2500000);
        state.setFrequency(100000000);
        state.setSampleRate(2400000);
        state.setPPM(-3);
        state.setOffset(1250000);
        CHECK(state.getFrequency() == 100000000);
        CHECK(state.getSampleRate() == 2400000);
        CHECK(state.getPPM() == -3);
        CHECK(state.getOffset() == 1250000);
    }

    TEST_CASE("snap clamps above one megahertz") {
        CoreStateImpl state(2500000);
        state.setFrequencySnap(100);
        CHECK(state.getFrequencySnap() == 100);
        state.setFrequencySnap(1000001);
        CHECK(state.getFrequencySnap() == 1000000);
        state.setFrequencySnap(1000000);
        CHECK(state.getFrequencySnap() == 1000000);
    }

    TEST_CASE("unzoomed view falls back to device values") {
        CoreStateImpl state(2500000);
        state.setFrequency(100000000);
        state.setSampleRate(2400000);
        long long center = 0;
        long long span = 0;
        CHECK(state.getMainViewWindow(center, span) == true);
        CHECK(center == 100000000);
        CHECK(span == 2400000);
    }

    TEST_CASE("zoomed view set, read, and reset") {
        CoreStateImpl state(2500000);
        state.setFrequency(100000000);
        state.setSampleRate(2400000);
        state.setMainZoomedView(100100000, 200000);
        long long center = 0;
        long long bandwidth = 0;
        CHECK(state.getMainZoomedView(center, bandwidth) == true);
        CHECK(center == 100100000);
        CHECK(bandwidth == 200000);
        long long span = 0;
        CHECK(state.getMainViewWindow(center, span) == true);
        CHECK(center == 100100000);
        CHECK(span == 200000);
        state.resetMainZoomedView();
        CHECK(state.getMainZoomedView(center, bandwidth) == false);
        CHECK(state.getMainViewWindow(center, span) == true);
        CHECK(center == 100000000);
        CHECK(span == 2400000);
    }
}
