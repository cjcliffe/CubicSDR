// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "viewmodel/WaterfallHover.h"

using namespace WaterfallHover;
using DemodMarkers::Sideband;

TEST_SUITE("WaterfallHover") {
    TEST_CASE("picks in-reach demod on bandwidth edge") {
        std::vector<HoverCandidate> candidates = {{100000000, 200000, Sideband::Both}, {100500000, 200000, Sideband::Both}};
        HoverPick pick = pickHover(candidates, 100100000, 2000000, 10000000);
        CHECK(pick.index == 0);
        CHECK(pick.zone == DragZone::BandwidthRight);
    }

    TEST_CASE("edge match overrides nearer center") {
        std::vector<HoverCandidate> candidates = {{100000000, 200000, Sideband::Both}, {100150000, 200000, Sideband::Both}};
        HoverPick pick = pickHover(candidates, 100100000, 2000000, 10000000);
        CHECK(pick.index == 0);
        CHECK(pick.zone == DragZone::BandwidthRight);
    }

    TEST_CASE("first candidate wins ties") {
        std::vector<HoverCandidate> candidates = {{100000000, 200000, Sideband::Both}, {100000000, 200000, Sideband::Both}};
        HoverPick pick = pickHover(candidates, 100010000, 2000000, 10000000);
        CHECK(pick.index == 0);
        CHECK(pick.zone == DragZone::Frequency);
    }

    TEST_CASE("buffer zone follows zoom level") {
        std::vector<HoverCandidate> candidates = {{100000000, 10000, Sideband::Both}};
        HoverPick wide = pickHover(candidates, 100006000, 2000000, 10000000);
        CHECK(wide.index == 0);
        CHECK(wide.zone == DragZone::BandwidthRight);
        HoverPick narrow = pickHover(candidates, 100006000, 200000, 10000000);
        CHECK(narrow.index == -1);
        CHECK(narrow.zone == DragZone::None);
    }

    TEST_CASE("sideband excludes wrong side") {
        std::vector<HoverCandidate> usb = {{100000000, 10000, Sideband::UpperOnly}};
        HoverPick below = pickHover(usb, 99997000, 2000000, 10000000);
        CHECK(below.index == -1);
        CHECK(below.zone == DragZone::None);
        HoverPick above = pickHover(usb, 100003000, 2000000, 10000000);
        CHECK(above.index == 0);
        CHECK(above.zone == DragZone::Frequency);
        std::vector<HoverCandidate> lsb = {{100000000, 10000, Sideband::LowerOnly}};
        HoverPick lsbAbove = pickHover(lsb, 100003000, 2000000, 10000000);
        CHECK(lsbAbove.index == -1);
        CHECK(lsbAbove.zone == DragZone::None);
    }

    TEST_CASE("center zone selects frequency drag") {
        std::vector<HoverCandidate> candidates = {{100000000, 300000, Sideband::Both}};
        HoverPick pick = pickHover(candidates, 100010000, 2000000, 10000000);
        CHECK(pick.index == 0);
        CHECK(pick.zone == DragZone::Frequency);
    }

    TEST_CASE("degenerate bandwidth keeps previous zone") {
        std::vector<HoverCandidate> candidates = {{100000000, -3000, Sideband::LowerOnly}};
        HoverPick pick = pickHover(candidates, 100000000, 2000000, 10000000);
        CHECK(pick.index == 0);
        CHECK(pick.zone == DragZone::KeepPrevious);
    }

    TEST_CASE("empty candidates miss") {
        std::vector<HoverCandidate> candidates;
        HoverPick pick = pickHover(candidates, 100000000, 2000000, 10000000);
        CHECK(pick.index == -1);
        CHECK(pick.zone == DragZone::None);
    }
}
