// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <atomic>

class FontScale {
public:
    enum Scale { SCALE_NORMAL, SCALE_MEDIUM, SCALE_LARGE };

    static void setScale(Scale scale);
    static Scale getScale();
    static double getScaleFactor();

private:
    static std::atomic<Scale> currentScale;
};
