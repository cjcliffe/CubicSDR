// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "viewmodel/SpectrumTicks.h"

using namespace SpectrumTicks;

TEST_SUITE("SpectrumTicks") {
    TEST_CASE("default step labels whole and fractional megahertz") {
        AxisGeometry geo = axisGeometry(100000000, 2000000, 1000.0, 500.0, 1.0);
        CHECK(geo.visualStep == 0.1);
        CHECK(geo.precision == 1);
        CHECK(geo.fontSize == 16);
        CHECK(geo.hPos == doctest::Approx(0.964));
        CHECK(geo.tickBase == doctest::Approx(0.99));
        CHECK(geo.ticks.size() >= 20);
        CHECK(geo.ticks.size() <= 21);
        CHECK(geo.ticks[0].x == -1.0);
        CHECK(geo.ticks[0].label == "99.0");
        CHECK(geo.ticks[0].major);
        CHECK(geo.ticks[1].label == "99.1");
        CHECK(!geo.ticks[1].major);
        for (size_t i = 1; i < geo.ticks.size(); i++) {
            CHECK(geo.ticks[i].x - geo.ticks[i - 1].x == doctest::Approx(0.1));
        }
        CHECK(geo.ticks.back().x > 0.99);
        CHECK(geo.ticks.back().x <= 1.0);
    }

    TEST_CASE("wide spans escalate to coarser steps") {
        AxisGeometry geo = axisGeometry(100000000, 10000000, 1000.0, 500.0, 1.0);
        CHECK(geo.visualStep == 0.5);
        CHECK(geo.precision == 1);
        CHECK(geo.ticks[0].label == "95.0");
        CHECK(geo.ticks[0].major);
        CHECK(geo.ticks[1].label == "95.5");
        CHECK(!geo.ticks[1].major);
        CHECK(geo.ticks[2].label == "96.0");
        CHECK(geo.ticks[2].major);
        AxisGeometry widest = axisGeometry(500000000, 1000000000, 1000.0, 500.0, 1.0);
        CHECK(widest.visualStep == 50.0);
    }

    TEST_CASE("narrow spans drop to fine steps with two decimals") {
        AxisGeometry geo = axisGeometry(100000000, 200000, 1000.0, 500.0, 1.0);
        CHECK(geo.visualStep == 0.01);
        CHECK(geo.precision == 2);
        CHECK(geo.ticks.size() >= 15);
        CHECK(geo.ticks.size() <= 25);
        for (auto &tick : geo.ticks) {
            size_t dot = tick.label.find('.');
            CHECK(dot != std::string::npos);
            CHECK(tick.label.size() == dot + 3);
        }
        CHECK(geo.ticks.front().x >= -1.0);
        CHECK(geo.ticks.back().x <= 1.0);
        for (size_t i = 1; i < geo.ticks.size(); i++) {
            CHECK(geo.ticks[i].x - geo.ticks[i - 1].x == doctest::Approx(0.1));
        }
    }

    TEST_CASE("negative frequencies are skipped and zero is snapped") {
        AxisGeometry geo = axisGeometry(0, 3000000, 1000.0, 500.0, 1.0);
        CHECK(geo.visualStep == 0.25);
        bool foundZero = false;
        for (auto &tick : geo.ticks) {
            bool nonNegative = tick.label.empty() || tick.label[0] != '-';
            CHECK(nonNegative);
            if (tick.label == "0.0") {
                foundZero = true;
                CHECK(tick.major);
                CHECK(std::fabs(tick.x) < 1e-9);
            }
        }
        CHECK(foundZero);
    }

    TEST_CASE("font size follows view height") {
        CHECK(axisGeometry(100000000, 2000000, 1000.0, 100.0, 1.0).fontSize == 12);
        CHECK(axisGeometry(100000000, 2000000, 1000.0, 135.0, 1.0).fontSize == 12);
        CHECK(axisGeometry(100000000, 2000000, 1000.0, 136.0, 1.0).fontSize == 16);
        CHECK(axisGeometry(100000000, 2000000, 1000.0, 500.0, 1.0).fontSize == 16);
        CHECK(axisGeometry(100000000, 2000000, 1000.0, 100.0, 1.0).hPos == doctest::Approx(0.84));
    }

    TEST_CASE("degenerate inputs yield empty geometry") {
        CHECK(axisGeometry(100000000, 0, 1000.0, 500.0, 1.0).empty());
        CHECK(axisGeometry(100000000, -1000, 1000.0, 500.0, 1.0).empty());
        CHECK(axisGeometry(100000000, 2000000, 0.0, 500.0, 1.0).empty());
        CHECK(axisGeometry(100000000, 2000000, 1000.0, 0.0, 1.0).empty());
    }

    TEST_CASE("db labels format ceiling and floor") {
        DbLabels labels = dbLabels(1000.0, 100.0, 1024, 0.0);
        CHECK(labels.ceil == "5.8dB");
        CHECK(labels.floor == "-14.2dB");
        DbLabels offset = dbLabels(1000.0, 100.0, 1024, 10.0);
        CHECK(offset.ceil == "15.8dB");
        CHECK(offset.floor == "-4.2dB");
    }

    TEST_CASE("db labels are empty without range or fft size") {
        DbLabels flat = dbLabels(5.0, 5.0, 1024, 0.0);
        CHECK(flat.ceil.empty());
        CHECK(flat.floor.empty());
        DbLabels noFft = dbLabels(1000.0, 100.0, 0, 0.0);
        CHECK(noFft.ceil.empty());
        CHECK(noFft.floor.empty());
    }

    TEST_CASE("small ranges use unit steps at full alpha") {
        GridGeometry geo = gridGeometry(0.0f, 10.0f);
        CHECK(geo.bands.size() == 1);
        CHECK(geo.bands[0].alpha == 1.0);
        CHECK(geo.bands[0].verts.size() == 48);
        CHECK(geo.bands[0].verts[0] == 0.0f);
        CHECK(geo.bands[0].verts[1] == doctest::Approx(0.1f));
        CHECK(geo.bands[0].verts[2] == 1.0f);
        CHECK(geo.bands[0].verts[5] == doctest::Approx(0.2f));
        CHECK(geo.bands[0].verts.back() == doctest::Approx(1.2f));
        for (size_t k = 0; k < geo.bands[0].verts.size() / 4; k++) {
            CHECK(geo.bands[0].verts[k * 4] == 0.0f);
            CHECK(geo.bands[0].verts[k * 4 + 2] == 1.0f);
            CHECK(geo.bands[0].verts[k * 4 + 1] == geo.bands[0].verts[k * 4 + 3]);
        }
    }

    TEST_CASE("large ranges use hundred steps") {
        GridGeometry geo = gridGeometry(0.0f, 1000.0f);
        CHECK(geo.bands.size() == 1);
        CHECK(geo.bands[0].alpha == 1.0);
        CHECK(geo.bands[0].verts.size() == 48);
        CHECK(geo.bands[0].verts[1] == doctest::Approx(0.1f));
    }

    TEST_CASE("alpha fades near band edges") {
        GridGeometry low = gridGeometry(0.0f, 95.0f);
        CHECK(low.bands.size() == 2);
        CHECK(low.bands[0].alpha == doctest::Approx(0.5));
        CHECK(low.bands[1].alpha == 1.0);
        CHECK(low.bands[0].verts.size() == 8);
        GridGeometry high = gridGeometry(0.0f, 145.0f);
        CHECK(high.bands.size() == 2);
        CHECK(high.bands[0].alpha == 1.0);
        CHECK(high.bands[1].alpha == doctest::Approx(0.5));
        GridGeometry edge = gridGeometry(0.0f, 90.0f);
        CHECK(edge.bands.size() == 2);
        CHECK(edge.bands[0].alpha == 0.0);
    }

    TEST_CASE("out-of-range yields no bands") {
        CHECK(gridGeometry(0.0f, 6000.0f).empty());
    }

    TEST_CASE("fill strip drops verticals to zero") {
        std::vector<float> points = {0.0f, 0.5f, 1.0f, 0.75f};
        std::vector<float> fill = fillStrip(points);
        CHECK(fill.size() == 8);
        CHECK(fill[0] == 0.0f);
        CHECK(fill[1] == 0.5f);
        CHECK(fill[2] == 0.0f);
        CHECK(fill[3] == 0.0f);
        CHECK(fill[4] == 1.0f);
        CHECK(fill[5] == 0.75f);
        CHECK(fill[6] == 1.0f);
        CHECK(fill[7] == 0.0f);
    }

    TEST_CASE("fill strip drops odd tail and zeroes it") {
        std::vector<float> points = {0.0f, 0.5f, 1.0f, 0.75f, 0.25f};
        std::vector<float> fill = fillStrip(points);
        CHECK(fill.size() == 10);
        CHECK(fill[4] == 1.0f);
        CHECK(fill[8] == 0.0f);
        CHECK(fill[9] == 0.0f);
        std::vector<float> empty;
        CHECK(fillStrip(empty).empty());
    }
}
