// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "viewmodel/ScopeCarousel.h"

using namespace ScopeCarousel;

TEST_SUITE("ScopeCarousel") {
    TEST_CASE("rest position shows scope only") {
        CarouselState state;
        CHECK(scopeVisibleStep(state, 0.4f));
        CHECK(!spectrumVisibleStep(state, 0.4f));
    }

    TEST_CASE("transition shows both panels") {
        CarouselState state;
        state.ctr = 1.8f;
        CHECK(scopeVisibleStep(state, 0.4f));
        CHECK(spectrumVisibleStep(state, 0.4f));
    }

    TEST_CASE("settled spectrum keeps scope drawn") {
        CarouselState state;
        state.ctr = -2.4f;
        CHECK(scopeVisibleStep(state, 0.4f));
        CHECK(spectrumVisibleStep(state, 0.4f));
    }

    TEST_CASE("rest eases toward target and snaps") {
        CarouselState state;
        state.ctr = 0.5f;
        restStep(state, 0.4f, false);
        CHECK(state.ctrTarget == 0.0f);
        CHECK(state.ctr == doctest::Approx(0.4f));
        for (int i = 0; i < 200; i++) {
            restStep(state, 0.4f, false);
        }
        CHECK(state.ctr == 0.0f);
    }

    TEST_CASE("rest clamps target range") {
        CarouselState high;
        high.ctr = 5.0f;
        restStep(high, 0.4f, false);
        CHECK(high.ctrTarget == 0.0f);
        CarouselState low;
        low.ctr = -5.0f;
        restStep(low, 0.4f, false);
        CHECK(low.ctrTarget == doctest::Approx(-2.4).epsilon(1e-5));
    }

    TEST_CASE("rest decays fling and kills small or wild values") {
        CarouselState glide;
        glide.dragAccel = 1.0f;
        restStep(glide, 0.4f, false);
        CHECK(glide.dragAccel == doctest::Approx(0.9f));
        CHECK(glide.ctr == doctest::Approx(0.9f));
        CarouselState small;
        small.dragAccel = 0.1f;
        restStep(small, 0.4f, false);
        CHECK(small.dragAccel == 0.0f);
        CarouselState wild;
        wild.ctr = 5.0f;
        wild.dragAccel = 1.0f;
        restStep(wild, 0.4f, false);
        CHECK(wild.dragAccel == 0.0f);
    }

    TEST_CASE("drag kicks velocity and position") {
        CarouselState state;
        dragStep(state, 0.25f);
        CHECK(state.dragAccel == 1.0f);
        CHECK(state.ctr == 1.0f);
        CarouselState back;
        dragStep(back, -0.25f);
        CHECK(back.dragAccel == -1.0f);
        CHECK(back.ctr == -1.0f);
    }

    TEST_CASE("panel tilt faces viewer") {
        CHECK(panelTiltDeg(0.0f) == 0.0f);
        CHECK(panelTiltDeg(1.2f) == doctest::Approx(-45.0).epsilon(1e-5));
        CHECK(panelTiltDeg(-1.2f) == doctest::Approx(45.0).epsilon(1e-5));
    }
}
