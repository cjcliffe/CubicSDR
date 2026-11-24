// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/SpectrumTicks.h"

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
}
