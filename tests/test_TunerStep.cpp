// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "doctest.h"
#include "util/TunerStep.h"

using namespace TunerStep;

TEST_SUITE("TunerStep") {
    TEST_CASE("plain stepping adds and subtracts powers of ten") {
        StepInput in;
        in.value = 100;
        in.digit = 0;
        in.up = true;
        CHECK(stepDigit(in) == 101);
        in.up = false;
        CHECK(stepDigit(in) == 99);
        in.value = 100;
        in.digit = 2;
        in.up = true;
        CHECK(stepDigit(in) == 200);
        in.up = false;
        CHECK(stepDigit(in) == 0);
        in.value = 0;
        CHECK(stepDigit(in) == -100);
        in.value = 100000000;
        in.digit = 7;
        in.up = true;
        CHECK(stepDigit(in) == 110000000);
        in.value = 0;
        in.digit = 10;
        CHECK(stepDigit(in) == 10000000000LL);
    }

    TEST_CASE("zeroing out truncates to the digit above") {
        StepInput in;
        in.zeroOut = true;
        in.value = 12345;
        in.digit = 1;
        in.up = true;
        CHECK(stepDigit(in) == 12300);
        in.up = false;
        CHECK(stepDigit(in) == 12300);
        in.digit = 0;
        CHECK(stepDigit(in) == 12340);
        in.value = -12345;
        in.digit = 1;
        CHECK(stepDigit(in) == -12300);
    }

    TEST_CASE("preventing carry wraps the digit instead") {
        StepInput in;
        in.preventCarry = true;
        in.value = 199;
        in.digit = 0;
        in.up = true;
        CHECK(stepDigit(in) == 190);
        in.value = 190;
        in.up = false;
        CHECK(stepDigit(in) == 199);
        in.value = 195;
        in.up = true;
        CHECK(stepDigit(in) == 196);
    }

    TEST_CASE("bottom region forces carry on small values") {
        StepInput in;
        in.preventCarry = true;
        in.bottom = true;
        in.value = 5;
        in.digit = 1;
        in.up = false;
        CHECK(stepDigit(in) == 95);
        in.bottom = false;
        CHECK(stepDigit(in) == -5);
    }

    TEST_CASE("doubling applies to bandwidth steps") {
        StepInput in;
        in.doubleAmount = true;
        in.value = 1000;
        in.digit = 2;
        in.up = true;
        CHECK(stepDigit(in) == 1200);
        in.preventCarry = true;
        in.value = 1900;
        CHECK(stepDigit(in) == 100);
    }

    TEST_CASE("wheel rotation accumulates into steps") {
        WheelSteps full = wheelSteps(0, 120, 120);
        CHECK(full.steps == 1);
        CHECK(full.rest == 0);
        WheelSteps partial = wheelSteps(0, 50, 120);
        CHECK(partial.steps == 0);
        CHECK(partial.rest == 50);
        WheelSteps carry = wheelSteps(50, 100, 120);
        CHECK(carry.steps == 1);
        CHECK(carry.rest == 30);
        WheelSteps neg = wheelSteps(0, -130, 120);
        CHECK(neg.steps == -1);
        CHECK(neg.rest == -10);
        WheelSteps negCarry = wheelSteps(-10, -120, 120);
        CHECK(negCarry.steps == -1);
        CHECK(negCarry.rest == -10);
    }

    TEST_CASE("drag accumulates past unit thresholds") {
        DragSteps calm = dragSteps(0.5f);
        CHECK(calm.up == 0);
        CHECK(calm.down == 0);
        CHECK(calm.rest == 0.5f);
        DragSteps up = dragSteps(2.7f);
        CHECK(up.up == 2);
        CHECK(up.down == 0);
        CHECK(up.rest == doctest::Approx(0.7f));
        DragSteps down = dragSteps(-2.7f);
        CHECK(down.up == 0);
        CHECK(down.down == 2);
        CHECK(down.rest == doctest::Approx(-0.7f));
        DragSteps edge = dragSteps(1.0f);
        CHECK(edge.up == 0);
        CHECK(edge.down == 0);
        CHECK(edge.rest == 1.0f);
    }
}
