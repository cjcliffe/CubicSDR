// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <cstdint>

class RateGate {
public:
    bool shouldFire(uint64_t nowMs, uint64_t intervalMs) {
        if (nowMs - lastFireMs >= intervalMs) {
            lastFireMs = nowMs;
            return true;
        }
        return false;
    }

private:
    uint64_t lastFireMs = 0;
};
