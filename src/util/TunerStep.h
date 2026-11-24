// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cmath>

namespace TunerStep {

struct StepInput {
    long long value = 0;
    int digit = 0;
    bool up = true;
    bool preventCarry = false;
    bool zeroOut = false;
    bool bottom = false;
    bool doubleAmount = false;
};

struct WheelSteps {
    int steps = 0;
    int rest = 0;
};

struct DragSteps {
    int up = 0;
    int down = 0;
    float rest = 0.0f;
};

inline long long stepDigit(const StepInput &in) {
    double exp = pow(10, in.digit);
    long long amount = in.up ? exp : -exp;
    if (in.doubleAmount) {
        amount *= 2;
    }
    long long stepped = in.value;
    if (in.zeroOut) {
        double intpart;
        (void)modf(stepped / (exp * 10), &intpart);
        stepped = intpart * exp * 10;
    } else if (in.preventCarry) {
        bool carried = (long long)((stepped) / (exp * 10)) != (long long)((stepped + amount) / (exp * 10)) || (in.bottom && stepped < exp);
        stepped += carried ? (9 * -amount) : amount;
    } else {
        stepped += amount;
    }
    return stepped;
}

inline WheelSteps wheelSteps(int accum, int rotation, int delta) {
    WheelSteps ws;
    accum += rotation;
    ws.steps = accum / delta;
    ws.rest = accum - ws.steps * delta;
    return ws;
}

inline DragSteps dragSteps(float accum) {
    DragSteps ds;
    while (accum > 1.0) {
        ds.up++;
        accum -= 1.0;
    }
    while (accum < -1.0) {
        ds.down++;
        accum += 1.0;
    }
    ds.rest = accum;
    return ds;
}

}
