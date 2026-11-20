// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "process/SpectrumDecimate.h"

TEST_SUITE("SpectrumDecimate") {
    TEST_CASE("peak wins over span") {
        std::vector<double> bins(64, 1.0);
        bins[30] = 9.0;
        std::vector<unsigned int> idxs;
        for (unsigned int i = 20; i < 40; i++) {
            idxs.push_back(i);
        }
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(9.0));
    }

    TEST_CASE("single bin passes through") {
        std::vector<double> bins(64, 1.0);
        bins[25] = 4.0;
        CHECK(reducePixelMax(bins, {25}, 0.5) == doctest::Approx(4.0));
    }

    TEST_CASE("out-of-range indexes read the floor") {
        std::vector<double> bins(64, 1.0);
        std::vector<unsigned int> idxs{64, 100, 4000000000u};
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(0.5));
    }

    TEST_CASE("bin zero is excluded") {
        std::vector<double> bins(64, 1.0);
        bins[0] = 50.0;
        std::vector<unsigned int> idxs{0, 10};
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(1.0));
    }

    TEST_CASE("empty span yields floor") {
        std::vector<double> bins(64, 1.0);
        std::vector<unsigned int> idxs;
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(0.5));
    }

    TEST_CASE("all below floor yields floor") {
        std::vector<double> bins(64, 0.1);
        std::vector<unsigned int> idxs{10, 20, 30};
        CHECK(reducePixelMax(bins, idxs, 0.5) == doctest::Approx(0.5));
    }
}
