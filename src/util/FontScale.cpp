// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "FontScale.h"

std::atomic<FontScale::Scale> FontScale::currentScale{FontScale::Scale::SCALE_NORMAL};

void FontScale::setScale(Scale scale) {
    if (scale < SCALE_NORMAL || scale > SCALE_LARGE) {
        scale = Scale::SCALE_NORMAL;
    }

    currentScale.store(scale);
}

FontScale::Scale FontScale::getScale() {
    return currentScale.load();
}

double FontScale::getScaleFactor() {
    Scale scale = currentScale.load();

    if (scale == SCALE_MEDIUM) {
        return 1.5;
    } else if (scale == SCALE_LARGE) {
        return 2.0;
    }

    return 1.0;
}
