// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

inline int waterfallMeterToLps(float meterValue) {
    return (int)(meterValue * meterValue + 0.5f);
}
