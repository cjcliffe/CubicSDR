// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Pure interaction state and button layout for mode selector widgets.
// Release only ever sets the change flag, and a disabled control is a no-op.

#pragma once

#include <cmath>
#include <vector>

namespace ModeSelector {

struct ReleaseResult {
    int selection = -1;
    bool changed = false;
};

struct SelectorCell {
    std::vector<float> outline;
    float labelY = 0.0f;
    int fontSize = 18;
};

inline int hoveredRow(float mouseY01, int numChoices, bool inView) {
    if (!inView || numChoices <= 0) {
        return -1;
    }

    float ypos = 1.0 - (mouseY01 * 2.0);
    int yval = (int)(((ypos + 1.0) / 2.0) * (float)numChoices);

    if (yval >= numChoices) {
        yval = numChoices - 1;
    }
    if (yval < 0) {
        yval = 0;
    }

    return yval;
}

inline ReleaseResult resolveRelease(int currentSelection, int hoveredSelection, bool toggleMode, bool isClick, bool disabled) {
    ReleaseResult out;
    if (disabled) {
        out.selection = currentSelection;
        return out;
    }
    int selectedButton = currentSelection;
    if (isClick) {
        selectedButton = hoveredSelection;
    }
    if (toggleMode && (currentSelection == selectedButton)) {
        selectedButton = -1;
    }
    if (currentSelection != selectedButton || (!toggleMode && selectedButton != -1 && isClick)) {
        out.changed = true;
    }
    out.selection = selectedButton;
    return out;
}

inline float pulsePhaseAt(long long milliSeconds) {
    return 6.2831853f * (float)(milliSeconds % 2000) / 2000.0f;
}

inline float pulseBlend(float phase) {
    return 0.5f + 0.5f * sinf(phase);
}

inline SelectorCell selectorCell(int c, int cMax, float viewWidth, float viewHeight, float padXPx, float padYPx) {
    SelectorCell cell;
    cell.fontSize = 18;
    if (viewWidth < 30 || viewHeight < 200) {
        cell.fontSize = 16;
    }
    float y = 1.0 - ((float)(c + 1) / (float)cMax * 2.0);
    float height = (2.0 / (float)cMax);
    float padX = (padXPx / viewWidth);
    float padY = (padYPx / viewHeight);
    cell.labelY = (float)(y + height / 2.0);
    cell.outline = {-1.0f + padX, y + padY, 1.0f - padX, y + padY, 1.0f - padX, y + height - padY, -1.0f + padX,
        y + height - padY};
    return cell;
}

}
