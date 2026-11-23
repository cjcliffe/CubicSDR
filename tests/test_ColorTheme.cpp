// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "visual/ColorTheme.h"

#include <set>
#include <string>

TEST_SUITE("ColorTheme") {
    TEST_CASE("default theme is selected") {
        ThemeMgr mgr;
        CHECK(mgr.getTheme() == COLOR_THEME_DEFAULT);
        CHECK(mgr.currentTheme != nullptr);
        CHECK(mgr.currentTheme->name == "Default");
    }

    TEST_CASE("all theme ids round-trip") {
        ThemeMgr mgr;
        std::set<std::string> names;
        for (int id = 0; id < COLOR_THEME_MAX; id++) {
            mgr.setTheme(id);
            CHECK(mgr.getTheme() == id);
            CHECK(mgr.currentTheme != nullptr);
            CHECK(!mgr.currentTheme->name.empty());
            names.insert(mgr.currentTheme->name);
        }
        CHECK(names.size() == COLOR_THEME_MAX);
    }

    TEST_CASE("unknown theme id keeps the current theme") {
        ThemeMgr mgr;
        mgr.setTheme(COLOR_THEME_BW);
        mgr.setTheme(99);
        CHECK(mgr.getTheme() == COLOR_THEME_BW);
        CHECK(mgr.currentTheme != nullptr);
        CHECK(mgr.currentTheme->name == "Black & White");
    }

    TEST_CASE("rgba values copy and scale") {
        RGBA4f color(0.5f, 0.25f, 1.0f, 0.5f);
        RGBA4f copy(color);
        CHECK(copy.r == 0.5f);
        CHECK(copy.g == 0.25f);
        CHECK(copy.b == 1.0f);
        CHECK(copy.a == 0.5f);
        RGBA4f dimmed = color * 0.5f;
        CHECK(dimmed.r == 0.25f);
        CHECK(dimmed.g == 0.125f);
        CHECK(dimmed.b == 0.5f);
        RGBA4f def;
        CHECK(def.r == 0.0f);
        CHECK(def.g == 0.0f);
        CHECK(def.b == 0.0f);
    }

    TEST_CASE("gradient generates the requested length") {
        Gradient gradient;
        gradient.generate(16);
        CHECK(gradient.getRed().size() == 0);
        gradient.addColors({GradientColor(0.0, 0.0, 0.0), GradientColor(1.0, 1.0, 1.0)});
        gradient.generate(16);
        CHECK(gradient.getRed().size() == 16);
        CHECK(gradient.getGreen().size() == 16);
        CHECK(gradient.getBlue().size() == 16);
        CHECK(gradient.getRed()[0] == 0.0f);
        CHECK(gradient.getRed()[15] == doctest::Approx(15.0f / 16.0f));
    }
}
