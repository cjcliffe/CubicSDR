// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <vector>

#include "doctest.h"
#include "Gradient.h"

TEST_SUITE("Gradient") {
    TEST_CASE("two-color gradient interpolates linearly") {
        Gradient g;
        g.addColor(GradientColor(0.0, 0.0, 0.0));
        g.addColor(GradientColor(1.0, 1.0, 1.0));
        g.generate(10);

        const std::vector<float> &red = g.getRed();
        REQUIRE(red.size() == 10);
        CHECK(red[0] == doctest::Approx(0.0).epsilon(1e-5));
        CHECK(red[5] == doctest::Approx(0.5).epsilon(1e-5));
        CHECK(red[9] == doctest::Approx(0.9).epsilon(1e-5));
        for (size_t i = 1; i < red.size(); i++) {
            CHECK(red[i] > red[i - 1]);
        }
        CHECK(g.getGreen().size() == 10);
        CHECK(g.getBlue().size() == 10);
    }

    TEST_CASE("output length matches the requested length") {
        Gradient g;
        g.addColor(GradientColor(1.0, 0.0, 0.0));
        g.addColor(GradientColor(0.0, 0.0, 1.0));
        g.generate(256);
        CHECK(g.getRed().size() == 256);
        CHECK(g.getGreen().size() == 256);
        CHECK(g.getBlue().size() == 256);
    }

    TEST_CASE("multi-stop gradient interpolates each segment") {
        Gradient g;
        g.addColor(GradientColor(1.0, 0.0, 0.0));
        g.addColor(GradientColor(0.0, 1.0, 0.0));
        g.addColor(GradientColor(0.0, 0.0, 1.0));
        g.generate(100);

        CHECK(g.getRed()[0] == doctest::Approx(1.0).epsilon(1e-5));
        CHECK(g.getRed()[49] == doctest::Approx(0.02).epsilon(1e-5));
        CHECK(g.getGreen()[50] == doctest::Approx(1.0).epsilon(1e-5));
        CHECK(g.getBlue()[99] == doctest::Approx(0.98).epsilon(1e-5));
    }

    TEST_CASE("out-of-range components clamp to the valid range") {
        Gradient g;
        g.addColor(GradientColor(2.0, -1.0, 0.5));
        g.addColor(GradientColor(0.0, 0.0, 0.0));
        g.generate(4);

        CHECK(g.getRed()[0] == doctest::Approx(1.0).epsilon(1e-5));
        CHECK(g.getGreen()[0] == doctest::Approx(0.0).epsilon(1e-5));
        CHECK(g.getBlue()[0] == doctest::Approx(0.5).epsilon(1e-5));
    }

    TEST_CASE("generate with fewer than two colors produces empty arrays") {
        Gradient empty;
        empty.generate(10);
        CHECK(empty.getRed().empty());
        CHECK(empty.getGreen().empty());
        CHECK(empty.getBlue().empty());

        Gradient single;
        single.addColor(GradientColor(1.0, 1.0, 1.0));
        single.generate(10);
        CHECK(single.getRed().empty());
        CHECK(single.getGreen().empty());
        CHECK(single.getBlue().empty());
    }

    TEST_CASE("clear resets state") {
        Gradient g;
        g.addColor(GradientColor(0.0, 0.0, 0.0));
        g.addColor(GradientColor(1.0, 1.0, 1.0));
        g.generate(10);
        REQUIRE(g.getRed().size() == 10);
        g.clear();
        g.generate(10);
        CHECK(g.getRed().empty());
        CHECK(g.getGreen().empty());
        CHECK(g.getBlue().empty());
    }
}
