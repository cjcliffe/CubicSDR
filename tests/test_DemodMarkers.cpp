// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/DemodMarkers.h"

using namespace DemodMarkers;

TEST_SUITE("DemodMarkers") {
    TEST_CASE("center maps to zero and edges to extremes") {
        CHECK(markerX(100000000, 100000000, 2000000) == 0.0f);
        CHECK(markerX(99000000, 100000000, 2000000) == -1.0f);
        CHECK(markerX(101000000, 100000000, 2000000) == 1.0f);
        CHECK(markerX(99500000, 100000000, 2000000) == -0.5f);
    }

    TEST_CASE("zero span guards to zero") {
        CHECK(markerX(100000000, 100000000, 0) == 0.0f);
    }

    TEST_CASE("sideband follows the demodulator type") {
        CHECK(sidebandForType("USB") == Sideband::UpperOnly);
        CHECK(sidebandForType("LSB") == Sideband::LowerOnly);
        CHECK(sidebandForType("FM") == Sideband::Both);
        CHECK(sidebandForType("") == Sideband::Both);
    }

    TEST_CASE("bandwidth splits by sideband") {
        MarkerInput in;
        in.freqHz = 100000000;
        in.bandwidthHz = 200000;
        in.viewCenterHz = 100000000;
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        MarkerGeometry dsb = markerGeometry(in);
        CHECK(dsb.ofs == doctest::Approx(0.1f));
        CHECK(dsb.ofsLeft == doctest::Approx(0.1f));
        CHECK(dsb.ofsRight == doctest::Approx(0.1f));
        in.sideband = Sideband::UpperOnly;
        MarkerGeometry usb = markerGeometry(in);
        CHECK(usb.ofsLeft == 0.0f);
        CHECK(usb.ofsRight == doctest::Approx(0.1f));
        in.sideband = Sideband::LowerOnly;
        MarkerGeometry lsb = markerGeometry(in);
        CHECK(lsb.ofsLeft == doctest::Approx(0.1f));
        CHECK(lsb.ofsRight == 0.0f);
    }

    TEST_CASE("label and band quads span the marker") {
        MarkerInput in;
        in.freqHz = 100000000;
        in.bandwidthHz = 200000;
        in.viewCenterHz = 100000000;
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        MarkerGeometry geo = markerGeometry(in);
        CHECK(geo.uxPos == 0.0f);
        CHECK(geo.labelHeight == doctest::Approx(20.0f / 600.0f));
        CHECK(geo.hPos == doctest::Approx(-1.0f + 20.0f / 600.0f));
        CHECK(geo.label.x0 == doctest::Approx(-0.1f));
        CHECK(geo.label.y0 == -1.0f);
        CHECK(geo.label.x1 == doctest::Approx(0.1f));
        CHECK(geo.label.y1 == doctest::Approx(-1.0f + 2.0f * 20.0f / 600.0f));
        CHECK(geo.band.y1 == 1.0f);
        CHECK(geo.narrowBand == false);
    }

    TEST_CASE("narrow bands flag below sixteen pixels") {
        MarkerInput in;
        in.freqHz = 100000000;
        in.bandwidthHz = 10000;
        in.viewCenterHz = 100000000;
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        CHECK(markerGeometry(in).narrowBand == true);
        in.bandwidthHz = 200000;
        CHECK(markerGeometry(in).narrowBand == false);
    }

    TEST_CASE("stacked labels sit three rows up") {
        MarkerInput in;
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        in.stackLabels = true;
        CHECK(markerGeometry(in).hPos == doctest::Approx(-1.0f + 3.0f * 20.0f / 600.0f));
    }

    TEST_CASE("label alignment follows sideband") {
        MarkerInput in;
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        in.sideband = Sideband::UpperOnly;
        CHECK(markerGeometry(in).align == LabelAlign::Left);
        in.sideband = Sideband::LowerOnly;
        CHECK(markerGeometry(in).align == LabelAlign::Right);
        in.sideband = Sideband::Both;
        CHECK(markerGeometry(in).align == LabelAlign::Center);
    }

    TEST_CASE("label prefix encodes flags in order") {
        MarkerInput in;
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        CHECK(markerGeometry(in).prefix == "");
        in.deltaLocked = true;
        in.recording = true;
        in.muted = true;
        CHECK(markerGeometry(in).prefix == "[VRM] ");
        in.muted = false;
        in.solo = true;
        CHECK(markerGeometry(in).prefix == "[VRS] ");
    }

    TEST_CASE("label background reflects state") {
        MarkerInput in;
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        RGBA4f plain = markerGeometry(in).bg;
        CHECK(plain.r == 0.0f);
        CHECK(plain.g == 0.0f);
        CHECK(plain.b == 0.0f);
        CHECK(plain.a == 0.35f);
        in.solo = true;
        RGBA4f solo = markerGeometry(in).bg;
        CHECK(solo.r == 0.8f);
        CHECK(solo.g == 0.8f);
        in.solo = false;
        in.muted = true;
        RGBA4f muted = markerGeometry(in).bg;
        CHECK(muted.r == 0.8f);
        CHECK(muted.g == 0.0f);
        in.muted = false;
        in.soloMode = true;
        CHECK(markerGeometry(in).bg.r == 0.2f);
    }

    TEST_CASE("recording pulse follows a two-second cycle") {
        MarkerInput in;
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        in.recording = true;
        in.timeMs = 0;
        CHECK(markerGeometry(in).bg.g == doctest::Approx(0.75f));
        in.timeMs = 500;
        CHECK(markerGeometry(in).bg.g == doctest::Approx(1.0f));
        in.timeMs = 1000;
        CHECK(markerGeometry(in).bg.g == doctest::Approx(0.75f));
        in.timeMs = 1500;
        CHECK(markerGeometry(in).bg.g == doctest::Approx(0.5f));
    }

    TEST_CASE("frequency label prints megahertz") {
        CHECK(freqLabelText(98000000) == "98.000000");
        CHECK(demodTypeLabel("FM", false) == "FM");
        CHECK(demodTypeLabel("USB", true) == "USB Lock");
    }

    TEST_CASE("degenerate views produce empty geometry") {
        MarkerInput in;
        in.freqHz = 100000000;
        in.bandwidthHz = 200000;
        in.viewCenterHz = 100000000;
        in.viewSpanHz = 0;
        in.viewWidthPx = 1280;
        in.viewHeightPx = 600;
        MarkerGeometry zero = markerGeometry(in);
        CHECK(zero.uxPos == 0.0f);
        CHECK(zero.ofs == 0.0f);
        CHECK(zero.narrowBand == false);
        in.viewSpanHz = 2000000;
        in.viewWidthPx = 0;
        CHECK(markerGeometry(in).uxPos == 0.0f);
    }
}
