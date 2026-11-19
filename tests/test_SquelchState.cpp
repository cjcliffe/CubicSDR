// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "demod/SquelchState.h"

#include <cmath>
#include <vector>

TEST_SUITE("Squelch") {
    TEST_CASE("starts closed and opens on sustained signal") {
        SquelchState sq;
        CHECK(sq.processBlock(-50.0, -70.0, 0.008) == true);
        for (int b = 0; b < 4; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        CHECK(sq.isSquelched() == false);
    }

    TEST_CASE("brief blip does not open") {
        SquelchState sq;
        sq.processBlock(-50.0, -70.0, 0.008);
        CHECK(sq.processBlock(-90.0, -70.0, 0.008) == true);
        CHECK(sq.isSquelched() == true);
    }

    TEST_CASE("hysteresis band holds open") {
        SquelchState sq;
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        for (int b = 0; b < 200; b++) {
            CHECK(sq.processBlock(-75.0, -70.0, 0.008) == false);
        }
        CHECK(sq.isSquelched() == false);
    }

    TEST_CASE("hang bridges dips then closes") {
        SquelchState sq;
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        for (int b = 0; b < 10; b++) {
            CHECK(sq.processBlock(-90.0, -70.0, 0.008) == false);
        }
        for (int b = 0; b < 100; b++) {
            sq.processBlock(-90.0, -70.0, 0.008);
        }
        CHECK(sq.isSquelched() == false);
        for (int b = 0; b < 30; b++) {
            sq.processBlock(-90.0, -70.0, 0.008);
        }
        CHECK(sq.isSquelched() == true);
    }

    TEST_CASE("hang resets on recovery") {
        SquelchState sq;
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-90.0, -70.0, 0.008);
        }
        for (int b = 0; b < 5; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        for (int b = 0; b < 10; b++) {
            CHECK(sq.processBlock(-90.0, -70.0, 0.008) == false);
        }
    }

    TEST_CASE("close then reopen cycles") {
        SquelchState sq;
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        for (int b = 0; b < 140; b++) {
            sq.processBlock(-90.0, -70.0, 0.008);
        }
        CHECK(sq.isSquelched() == true);
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        CHECK(sq.isSquelched() == false);
    }

    TEST_CASE("reset closes") {
        SquelchState sq;
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        sq.reset();
        CHECK(sq.isSquelched() == true);
    }

    TEST_CASE("zero dt never advances") {
        SquelchState sq;
        for (int b = 0; b < 100; b++) {
            CHECK(sq.processBlock(-50.0, -70.0, 0.0) == true);
        }
    }

    TEST_CASE("in-band hover holds open") {
        SquelchState sq;
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        for (int b = 0; b < 400; b++) {
            CHECK(sq.processBlock(-72.0, -70.0, 0.008) == false);
        }
        CHECK(sq.isSquelched() == false);
    }

    TEST_CASE("straddling noise trends closed") {
        SquelchState sq;
        for (int b = 0; b < 10; b++) {
            sq.processBlock(-50.0, -70.0, 0.008);
        }
        for (int b = 0; b < 100; b++) {
            sq.processBlock((b % 2 == 0) ? -90.0 : -72.0, -70.0, 0.008);
        }
        CHECK(sq.isSquelched() == false);
        for (int b = 0; b < 300; b++) {
            sq.processBlock((b % 2 == 0) ? -90.0 : -72.0, -70.0, 0.008);
        }
        CHECK(sq.isSquelched() == true);
    }

    TEST_CASE("steal takes empty or silent focus") {
        CHECK(squelchShouldSteal(true, false, false, false) == true);
        CHECK(squelchShouldSteal(true, false, true, false) == true);
    }

    TEST_CASE("steal never preempts live stream") {
        CHECK(squelchShouldSteal(true, false, true, true) == false);
    }

    TEST_CASE("steal respects busy and non-solo") {
        CHECK(squelchShouldSteal(true, true, false, false) == false);
        CHECK(squelchShouldSteal(false, false, false, false) == false);
        CHECK(squelchShouldSteal(false, false, true, false) == false);
    }
}

TEST_SUITE("SquelchFade") {
    TEST_CASE("steady states pass through or drop") {
        SquelchFade fade(48000.0);
        std::vector<float> v(512, 1.0f);
        CHECK(fade.processBlock(&v[0], v.size(), true) == true);
        for (int b = 0; b < 3; b++) {
            std::fill(v.begin(), v.end(), 1.0f);
            fade.processBlock(&v[0], v.size(), false);
        }
        CHECK(fade.processBlock(&v[0], v.size(), false) == false);
        for (size_t i = 0; i < v.size(); i++) {
            CHECK(v[i] == 1.0f);
        }
    }

    TEST_CASE("close ramps to exact silence") {
        SquelchFade fade(48000.0);
        std::vector<float> v(512, 1.0f);
        fade.processBlock(&v[0], v.size(), false);
        std::fill(v.begin(), v.end(), 1.0f);
        CHECK(fade.processBlock(&v[0], v.size(), true) == true);
        CHECK(v[0] < 1.0f);
        CHECK(v[0] > 0.0f);
        for (size_t i = 240; i < v.size(); i++) {
            CHECK(v[i] == 0.0f);
        }
    }

    TEST_CASE("reversal stays continuous") {
        SquelchFade fade(48000.0);
        std::vector<float> v(512, 1.0f);
        fade.processBlock(&v[0], v.size(), false);
        std::fill(v.begin(), v.end(), 1.0f);
        fade.processBlock(&v[0], 100, true);
        float prev = v[99];
        float peak = 0.0f;
        std::fill(v.begin(), v.end(), 1.0f);
        fade.processBlock(&v[0], v.size(), false);
        for (size_t i = 0; i < v.size(); i++) {
            float d = fabsf(v[i] - prev);
            if (d > peak) {
                peak = d;
            }
            prev = v[i];
        }
        CHECK(peak < 0.05f);
        CHECK(v[v.size() - 1] == 1.0f);
    }

    TEST_CASE("reset snaps gain") {
        SquelchFade fade(48000.0);
        std::vector<float> v(512, 1.0f);
        fade.reset(false);
        CHECK(fade.processBlock(&v[0], v.size(), false) == false);
        CHECK(v[0] == 1.0f);
        fade.reset(true);
        CHECK(fade.processBlock(&v[0], v.size(), true) == true);
    }

    TEST_CASE("empty block holds state") {
        SquelchFade fade(48000.0);
        std::vector<float> v(1, 1.0f);
        CHECK(fade.processBlock(&v[0], 0, true) == true);
        fade.reset(false);
        CHECK(fade.processBlock(&v[0], 0, false) == false);
    }

    TEST_CASE("snap places threshold above visible level") {
        CHECK(snapSquelchAbove(-70.0f, -100.0f, 0.0f) == doctest::Approx(-64.0f));
        CHECK(snapSquelchAbove(-70.0f, -100.0f, 0.0f, 2.0f) == doctest::Approx(-68.0f));
    }

    TEST_CASE("snap clamps to the meter scale") {
        CHECK(snapSquelchAbove(-1.0f, -100.0f, 0.0f) == doctest::Approx(0.0f));
        CHECK(snapSquelchAbove(-150.0f, -100.0f, 0.0f) == doctest::Approx(-100.0f));
    }
}
