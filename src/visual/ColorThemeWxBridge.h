// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <wx/colour.h>

#include <algorithm>

#include "ColorTheme.h"

inline wxColour rgbaToWxColour(const RGBA4f &color) {
    return wxColour((unsigned char)std::min(color.r * 255.0, 255.0),
                    (unsigned char)std::min(color.g * 255.0, 255.0),
                    (unsigned char)std::min(color.b * 255.0, 255.0));
}
