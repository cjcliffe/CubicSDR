// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "viewmodel/WaterfallDrag.h"

using namespace WaterfallDrag;
using DemodMarkers::Sideband;

TEST_SUITE("WaterfallDrag") {
    TEST_CASE("bandwidth drag steps by full width") {
        CHECK(bandwidthDragStep(0.25f, 2000000, false, 200000, 500, 2000000) == 1200000);
        CHECK(bandwidthDragStep(0.25f, 2000000, true, 1200000, 500, 1000000) == 200000);
        CHECK(bandwidthDragStep(0.5f, 2000000, false, 900000, 500, 1000000) == 1000000);
        CHECK(bandwidthDragStep(-0.25f, 2000000, false, 200000, 500, 1000000) == 500);
    }

    TEST_CASE("snap rounds deltas to grid") {
        CHECK(snapDelta(1500, 1000) == 2000);
        CHECK(snapDelta(1400, 1000) == 1000);
        CHECK(snapDelta(-1500, 1000) == -2000);
        CHECK(snapDelta(1500, 1) == 1500);
        CHECK(snapDelta(1500, 0) == 1500);
        CHECK(snapDelta(0, 1000) == 0);
    }

    TEST_CASE("drag offset anchors pointer to demod") {
        CHECK(dragOffset(0.5f, 2000000, 100000000, 100000000) == 0);
        CHECK(dragOffset(0.75f, 2000000, 100000000, 100000000) == 500000);
        CHECK(dragOffset(0.0f, 2000000, 100000000, 100000000) == -1000000);
    }

    TEST_CASE("wheel zoom steps by notches") {
        CHECK(wheelZoom(120, 120) == doctest::Approx(0.96).epsilon(1e-5));
        CHECK(wheelZoom(240, 120) == doctest::Approx(0.92).epsilon(1e-5));
        CHECK(wheelZoom(-120, 120) == doctest::Approx(1.04).epsilon(1e-5));
        CHECK(wheelZoom(120, 0) == doctest::Approx(0.96).epsilon(1e-5));
        CHECK(wheelZoom(0, 120) == 1.0f);
    }

    TEST_CASE("glide zoom accelerates with distance") {
        CHECK(glideZoom(500, 400, 1000) == doctest::Approx(0.998).epsilon(1e-5));
        CHECK(glideZoom(500, 600, 1000) == doctest::Approx(1.002).epsilon(1e-5));
        CHECK(glideZoom(500, 500, 1000) == 1.0f);
        CHECK(glideZoom(500, 300, 1000) == doctest::Approx(0.992).epsilon(1e-5));
    }

    TEST_CASE("click places frequency at pointer") {
        CHECK(clickPlaceFreq(0.5f, 100000000, 2000000, false, 0, 0) == 100000000);
        CHECK(clickPlaceFreq(0.75f, 100000000, 2000000, false, 0, 0) == 100500000);
        CHECK(clickPlaceFreq(0.75f, 100000000, 2000000, true, 100000000, 1) == 100500000);
    }

    TEST_CASE("click snap rounds delta or absolute") {
        CHECK(clickPlaceFreq(0.5008f, 100000000, 2000000, true, 100000000, 1000) == 100002000);
        CHECK(clickPlaceFreq(0.5008f, 100000000, 2000000, false, 0, 1000) == 100002000);
        CHECK(clickPlaceFreq(0.75f, 100000400, 2000000, false, 0, 1000) == 100500000);
    }

    TEST_CASE("range drag centers on midpoint") {
        RangeDrag range = rangeDrag(0.25f, 0.75f, 0.5f, Sideband::Both, 100000000, 2000000, 0, 500);
        CHECK(range.freqHz == 100000000);
        CHECK(range.bandwidthHz == 1000000);
    }

    TEST_CASE("range drag pins sideband edge") {
        RangeDrag lsb = rangeDrag(0.25f, 0.75f, 0.5f, Sideband::LowerOnly, 100000000, 2000000, 0, 500);
        CHECK(lsb.freqHz == 100500000);
        CHECK(lsb.bandwidthHz == 2000000);
        RangeDrag usb = rangeDrag(0.75f, 0.25f, -0.5f, Sideband::UpperOnly, 100000000, 2000000, 0, 500);
        CHECK(usb.freqHz == 99500000);
        CHECK(usb.bandwidthHz == 2000000);
    }

    TEST_CASE("range drag clamps tiny widths and snaps") {
        RangeDrag tiny = rangeDrag(0.5f, 0.5001f, 0.0001f, Sideband::Both, 100000000, 2000000, 0, 500);
        CHECK(tiny.bandwidthHz == 500);
        CHECK(tiny.freqHz > 100000000);
        CHECK(tiny.freqHz < 100000200);
        RangeDrag snapped = rangeDrag(0.25f, 0.75f, 0.5f, Sideband::Both, 100000400, 2000000, 1000, 500);
        CHECK(snapped.freqHz == 100000000);
        CHECK(snapped.bandwidthHz == 1000000);
    }

    TEST_CASE("frequency maps across span") {
        CHECK(freqAtX(0.5f, 100000000, 2000000) == 100000000);
        CHECK(freqAtX(0.0f, 100000000, 2000000) == 99000000);
        CHECK(freqAtX(1.0f, 100000000, 2000000) == 101000000);
        CHECK(freqAtX(0.75f, 100000000, 2000000) == 100500000);
    }
}
