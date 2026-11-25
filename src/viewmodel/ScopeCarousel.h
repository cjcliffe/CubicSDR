// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Pure panel-carousel animation state for the demod scope view.
// Visibility compares the scaled position against the unscaled target index, so a
// settled spectrum position keeps the scope panel drawn.

#pragma once

#include <cmath>

namespace ScopeCarousel {

struct CarouselState {
    float ctr = 0.0f;
    float ctrTarget = 0.0f;
    float dragAccel = 0.0f;
};

inline float carouselInterval(float panelSpacing) {
    return 2.0 + panelSpacing;
}

inline bool scopeVisibleStep(CarouselState &state, float panelSpacing) {
    float panelInterval = 2.0 + panelSpacing;
    state.ctrTarget = std::abs(round(state.ctr / panelInterval));
    if (state.ctrTarget == 0 || state.dragAccel || (state.ctr != state.ctrTarget)) {
        return true;
    }
    return false;
}

inline bool spectrumVisibleStep(CarouselState &state, float panelSpacing) {
    float panelInterval = 2.0 + panelSpacing;
    state.ctrTarget = std::abs(round(state.ctr / panelInterval));
    if (state.ctrTarget == 1 || state.dragAccel || (state.ctr != state.ctrTarget)) {
        return true;
    }
    return false;
}

inline void restStep(CarouselState &state, float panelSpacing, bool mouseDown) {
    if (mouseDown) {
        return;
    }
    float panelInterval = 2.0 + panelSpacing;
    state.ctrTarget = round(state.ctr / panelInterval);
    if (state.ctrTarget < -1.0) {
        state.ctrTarget = -1.0;
    } else if (state.ctrTarget > 0.0) {
        state.ctrTarget = 0.0;
    }
    state.ctrTarget *= panelInterval;
    if (!state.dragAccel) {
        if (state.ctr != state.ctrTarget) {
            state.ctr += (state.ctrTarget - state.ctr) * 0.2;
        }
        if (std::abs(state.ctr - state.ctrTarget) < 0.001) {
            state.ctr = state.ctrTarget;
        }
    } else {
        state.dragAccel -= state.dragAccel * 0.1;
        if ((std::abs(state.dragAccel) < 0.2) || (state.ctr < (state.ctrTarget - panelInterval / 2.0)) ||
            (state.ctr > (state.ctrTarget + panelInterval / 2.0))) {
            state.dragAccel = 0;
        } else {
            state.ctr += state.dragAccel;
        }
    }
}

inline void dragStep(CarouselState &state, float deltaMouseX) {
    state.dragAccel = 4.0 * deltaMouseX;
    state.ctr += state.dragAccel;
}

inline float panelTiltDeg(float posX) {
    float roty = (float)atan2(posX, 1.2);
    return -(roty * (180.0 / 3.1415926535897931));
}

}
