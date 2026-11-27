// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "MeterCanvas.h"

#include <imgui.h>

#include "ui/imgui/imgui_bgfx.h"
#include "ColorTheme.h"
#include "CubicSDR.h"
#include "AppFrame.h"
#include "SquelchState.h"

#include <algorithm>
#include <string>

namespace {

struct ThemeColor {
    float r, g, b;
};

uint8_t clampByte(float v) {
    if (v < 0.0f) {
        v = 0.0f;
    }
    if (v > 1.0f) {
        v = 1.0f;
    }
    return (uint8_t)(v * 255.0f);
}

ImU32 packColor(float r, float g, float b, float a = 1.0f) {
    return IM_COL32(clampByte(r), clampByte(g), clampByte(b), clampByte(a));
}

uint32_t packClear(const ThemeColor &bg) {
    return ((uint32_t)clampByte(bg.r) << 24) | ((uint32_t)clampByte(bg.g) << 16)
        | ((uint32_t)clampByte(bg.b) << 8) | 0xFFu;
}

struct PanelFrame {
    ImDrawList *dl;
    float ox, oy, w, h;
    float px(float x) const {
        return ox + (x + 1.0f) * 0.5f * w;
    }
    float py(float y) const {
        return oy + (1.0f - y) * 0.5f * h;
    }
};

void emitHBar(
    const PanelFrame &f, float x0, float x1, float yTopPx, float yBotPx, ImU32 edgeL, ImU32 mid, ImU32 edgeR) {
    float xc = (x0 + x1) * 0.5f;
    f.dl->AddRectFilledMultiColor(ImVec2(x0, yTopPx), ImVec2(xc, yBotPx), edgeL, mid, mid, edgeL);
    f.dl->AddRectFilledMultiColor(ImVec2(xc, yTopPx), ImVec2(x1, yBotPx), mid, edgeR, edgeR, mid);
}

struct MeterBand {
    float frac;
    ThemeColor color;
};

} // namespace

MeterCanvas::MeterCanvas(wxWindow *parent, const char *viewName) : BgfxCanvas(parent), windowName(viewName) {
}

MeterCanvas::~MeterCanvas() = default;

void MeterCanvas::setLevel(float level_in) {
    level = level_in;
    Refresh();
}

float MeterCanvas::getLevel() const {
    return level;
}

void MeterCanvas::setMax(float max_in) {
    levelMax = max_in;
    Refresh();
}

void MeterCanvas::setMin(float min_in) {
    levelMin = min_in;
    Refresh();
}

void MeterCanvas::setUserInputValue(float slider_in) {
    userInputValue = slider_in;
    Refresh();
}

void MeterCanvas::setInputValue(float slider_in) {
    userInputValue = inputValue = slider_in;
    Refresh();
}

void MeterCanvas::setDefaultValue(float default_in) {
    defaultValue = default_in;
    hasDefault = true;
}

bool MeterCanvas::inputChanged() const {
    return (inputValue != userInputValue);
}

float MeterCanvas::getInputValue() {
    inputValue = userInputValue;
    return userInputValue;
}

void MeterCanvas::setShowUserInput(bool showUserInput_in) {
    showUserInput = showUserInput_in;
}

void MeterCanvas::setSnapOffset(float snapOffset_in) {
    snapOffset = snapOffset_in;
}

void MeterCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

void MeterCanvas::resetToDefault() {
    if (hasDefault) {
        userInputValue = defaultValue;
        Refresh();
    }
}

void MeterCanvas::meterWheelStep(int rotation) {
    float span = levelMax - levelMin;
    float movement = 3.0f * (float)rotation;
    float currentValue = showUserInput ? userInputValue : level;
    currentValue = currentValue + ((movement / 100.0f) * (span / 100.0f));
    if (currentValue > levelMax) {
        currentValue = levelMax;
    }
    if (currentValue < levelMin) {
        currentValue = levelMin;
    }
    userInputValue = currentValue;
}

void MeterCanvas::drawView(const BgfxEdges &edges, float mx, float my) {
    wxSize size = GetClientSize();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)size.x, (float)size.y), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin(windowName.c_str(), nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    bool hovered = isMouseInView() && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    float my01 = 1.0f - (my - origin.y) / wsize.y;
    float span = levelMax - levelMin;

    if (edges.mouseLeft) {
        pressedHere = false;
        rightPressedHere = false;
    } else {
        if (edges.leftPressed && hovered) {
            if (edges.leftClicks >= 2) {
                if (hasDefault) {
                    resetToDefault();
                    suppressNextRelease = true;
                }
            } else {
                userInputValue = my01 * span + levelMin;
                pressedHere = true;
            }
        }
        if (pressedHere && hovered) {
            userInputValue = my01 * span + levelMin;
        }
        if (edges.leftReleased) {
            if (suppressNextRelease) {
                suppressNextRelease = false;
            } else if (pressedHere && hovered) {
                userInputValue = my01 * span + levelMin;
            }
            pressedHere = false;
        }
        if (edges.rightPressed && hovered) {
            rightPressedHere = true;
        }
        if (edges.rightReleased) {
            if (rightPressedHere && hovered && showUserInput) {
                userInputValue = snapSquelchAbove(level, levelMin, levelMax, snapOffset);
            }
            rightPressedHere = false;
        }
        if (edges.middlePressed && hovered) {
            resetToDefault();
        }
        if (hovered) {
            if (edges.wheelRotV != 0) {
                meterWheelStep(edges.wheelRotV);
            }
            if (edges.wheelRotH != 0) {
                meterWheelStep(edges.wheelRotH);
            }
        }
    }

    ColorTheme *theme = ThemeMgr::mgr.currentTheme;
    ThemeColor bg{theme->generalBackground.r, theme->generalBackground.g, theme->generalBackground.b};
    ThemeColor levelCol{theme->meterLevel.r, theme->meterLevel.g, theme->meterLevel.b};
    ThemeColor valueCol{theme->meterValue.r, theme->meterValue.g, theme->meterValue.b};
    imguiSetClearColor(packClear(bg));

    float levelPos = span > 0.0f ? (level - levelMin) / span : 0.0f;
    float inputPos = span > 0.0f ? (userInputValue - levelMin) / span : 0.0f;
    levelPos = std::min(1.0f, std::max(0.0f, levelPos));
    inputPos = std::min(1.0f, std::max(0.0f, inputPos));
    MeterBand bands[4];
    int bandCount = 0;
    bands[bandCount++] = {1.0f, bg};
    if (hovered) {
        bands[bandCount++] = {my01, {0.4f, 0.4f, 0.4f}};
    }
    bands[bandCount++] = {levelPos, levelCol};
    if (showUserInput) {
        bands[bandCount++] = {inputPos, valueCol};
    }
    for (int i = 1; i < bandCount; i++) {
        MeterBand key = bands[i];
        int j = i - 1;
        while (j >= 0 && bands[j].frac > key.frac) {
            bands[j + 1] = bands[j];
            j--;
        }
        bands[j + 1] = key;
    }
    PanelFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, wsize.x, wsize.y};
    float prev = 0.0f;
    for (int i = 0; i < bandCount; i++) {
        float top = bands[i].frac;
        if (top > prev) {
            float er = bg.r, eg = bg.g, eb = bg.b;
            float cr = bg.r, cg = bg.g, cb = bg.b;
            for (int j = 0; j < bandCount; j++) {
                if (bands[j].frac >= top) {
                    er += bands[j].color.r * 0.65f;
                    eg += bands[j].color.g * 0.65f;
                    eb += bands[j].color.b * 0.65f;
                    cr += bands[j].color.r;
                    cg += bands[j].color.g;
                    cb += bands[j].color.b;
                }
            }
            emitHBar(f, f.px(-1.0f), f.px(1.0f), f.py(-1.0f + 2.0f * top), f.py(-1.0f + 2.0f * prev),
                packColor(er, eg, eb), packColor(cr, cg, cb), packColor(er, eg, eb));
            prev = top;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    if (!pressedHere && !helpTip.empty() && hovered) {
        wxGetApp().getAppFrame()->setStatusText(this, helpTip);
    }
}

void MeterCanvas::onEnterView() {
    SetCursor(wxCURSOR_CROSS);
#ifdef _WIN32
    if (wxGetApp().getAppFrame()->canFocus()) {
        SetFocus();
    }
#endif
}

void MeterCanvas::onLeaveView() {
    SetCursor(wxCURSOR_CROSS);
}
