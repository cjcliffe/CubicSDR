// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "GainCanvas.h"

#include <imgui.h>

#include "ui/imgui/imgui_bgfx.h"
#include "GainSlider.h"
#include "ColorTheme.h"
#include "CubicSDR.h"
#include "AppFrame.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

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

float labelFontSize(float bandPx) {
    if (bandPx <= 16.0f) {
        return 12.0f;
    }
    if (bandPx <= 18.0f) {
        return 16.0f;
    }
    if (bandPx <= 24.0f) {
        return 18.0f;
    }
    if (bandPx <= 32.0f) {
        return 24.0f;
    }
    if (bandPx <= 48.0f) {
        return 32.0f;
    }
    return 48.0f;
}

void centerText(const PanelFrame &f, const char *text, float cxPx, float cyPx, float fontPx) {
    ImGui::PushFont(imguiMonoFont(), fontPx);
    ImVec2 sz = ImGui::CalcTextSize(text);
    f.dl->AddText(ImVec2(cxPx - sz.x * 0.5f, cyPx - sz.y * 0.5f), IM_COL32_WHITE, text);
    ImGui::PopFont();
}

void emitHBar(
    const PanelFrame &f, float x0, float x1, float yTopPx, float yBotPx, ImU32 edgeL, ImU32 mid, ImU32 edgeR) {
    float xc = (x0 + x1) * 0.5f;
    f.dl->AddRectFilledMultiColor(ImVec2(x0, yTopPx), ImVec2(xc, yBotPx), edgeL, mid, mid, edgeL);
    f.dl->AddRectFilledMultiColor(ImVec2(xc, yTopPx), ImVec2(x1, yBotPx), mid, edgeR, edgeR, mid);
}

} // namespace

GainCanvas::GainCanvas(wxWindow *parent) : BgfxCanvas(parent), numGains(1), gainPressedHere(false) {
    userGainAsChanged = false;
}

GainCanvas::~GainCanvas() = default;

void GainCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

void GainCanvas::updateGainUI() {
    SDRDeviceInfo *devInfo = wxGetApp().getDevice();
    if (devInfo == nullptr) {
        return;
    }
    DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(devInfo->getDeviceId());
    gains = devInfo->getGains(SOAPY_SDR_RX, 0);
    numGains = (float)gains.size();
    if (!numGains) {
        return;
    }
    gainStrips.clear();
    for (const auto &gi : gains) {
        GainStrip strip;
        strip.name = gi.first;
        strip.low = (float)gi.second.minimum();
        strip.high = (float)gi.second.maximum();
        strip.value = devConfig->getGain(gi.first, wxGetApp().getGain(gi.first));
        gainStrips.push_back(strip);
    }
    Refresh();
}

bool GainCanvas::updateGainValues() {
    bool isRefreshNeeded = false;
    SDRDeviceInfo *devInfo = wxGetApp().getDevice();
    if (devInfo == nullptr || !devInfo->isActive()) {
        return false;
    }
    DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(devInfo->getDeviceId());
    gains = devInfo->getGains(SOAPY_SDR_RX, 0);
    size_t numGainsToRefresh = std::min(gains.size(), gainStrips.size());
    size_t panelIndex = 0;
    for (const auto &gi : gains) {
        if (panelIndex >= numGainsToRefresh) {
            break;
        }
        if (!gainStrips[panelIndex].changed) {
            float actualRoundedGain = GainSlider::roundGain(devInfo->getCurrentGain(SOAPY_SDR_RX, 0, gi.first));
            if (GainSlider::gainDisplayDiffers(actualRoundedGain, gainStrips[panelIndex].value)) {
                gainStrips[panelIndex].value = actualRoundedGain;
                devConfig->setGain(gi.first, actualRoundedGain);
                isRefreshNeeded = true;
            }
        }
        panelIndex++;
    }
    return isRefreshNeeded;
}

void GainCanvas::frameUpdate() {
    bool areGainsChangedHere = false;
    for (auto &g : gainStrips) {
        if (g.changed) {
            areGainsChangedHere = true;
            wxGetApp().setGain(g.name, (int)(g.value));
            wxGetApp().notifyMainUIOfDeviceChange(false);
            g.changed = false;
        }
    }
    if (areGainsChangedHere) {
        userGainAsChanged = true;
        userGainAsChangedDelayTimer.start();
    } else {
        userGainAsChangedDelayTimer.update();
        if (!userGainAsChanged || (userGainAsChanged && userGainAsChangedDelayTimer.getMilliseconds() > 150)) {
            if (updateGainValues()) {
                Refresh();
            }
            userGainAsChanged = false;
        }
    }
}

void GainCanvas::setLevel(float mx01, float my01) {
    if (gainStrips.empty()) {
        return;
    }
    GainSlider::GainStrip strip = GainSlider::gainStripLayout((int)gainStrips.size());
    float nx = mx01 * 2.0f - 1.0f;
    float ny = my01 * 2.0f - 1.0f;
    wxSize size = GetClientSize();
    GainSlider::LabelLayout layout = GainSlider::meterLabelLayout((double)size.y, 1.0, 18.0);
    float bgH = (float)layout.bgH;
    for (size_t i = 0; i < gainStrips.size(); i++) {
        float midX = GainSlider::gainBarMidX(strip, (int)i);
        float lx = (nx - midX) / strip.barWidth;
        float ly = ny / bgH;
        if (lx >= -1.0f && lx <= 1.0f && ly >= -1.0f && ly <= 1.0f) {
            gainStrips[i].value =
                GainSlider::clampGain(GainSlider::hitValue(ly, gainStrips[i].low, gainStrips[i].high),
                    gainStrips[i].low, gainStrips[i].high);
            gainStrips[i].changed = true;
            break;
        }
    }
}

void GainCanvas::drawView(const BgfxEdges &edges, float mx, float my) {
    wxSize size = GetClientSize();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)size.x, (float)size.y), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Gain", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    bool hovered = isMouseInView() && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    float mx01 = (mx - origin.x) / wsize.x;
    float my01 = 1.0f - (my - origin.y) / wsize.y;

    GainSlider::GainStrip strip = GainSlider::gainStripLayout((int)gainStrips.size() > 0 ? (int)gainStrips.size() : 1);
    GainSlider::LabelLayout layout = GainSlider::meterLabelLayout((double)wsize.y, 1.0, 18.0);
    float bgH = (float)layout.bgH;

    if (edges.mouseLeft) {
        gainPressedHere = false;
        for (auto &g : gainStrips) {
            g.highlightVisible = false;
        }
    } else {
        float nx = mx01 * 2.0f - 1.0f;
        float ny = my01 * 2.0f - 1.0f;
        for (size_t i = 0; i < gainStrips.size(); i++) {
            float midX = GainSlider::gainBarMidX(strip, (int)i);
            float lx = (nx - midX) / strip.barWidth;
            float ly = ny / bgH;
            if (hovered && lx >= -1.0f && lx <= 1.0f && ly >= -1.0f && ly <= 1.0f) {
                gainStrips[i].highlight =
                    GainSlider::clampGain(GainSlider::hitValue(ly, gainStrips[i].low, gainStrips[i].high),
                        gainStrips[i].low, gainStrips[i].high);
                gainStrips[i].highlightVisible = true;
                wxGetApp().setActiveGainEntry(gainStrips[i].name);
            } else {
                gainStrips[i].highlightVisible = false;
            }
        }
    }

    if (edges.leftPressed && hovered) {
        gainPressedHere = true;
    }
    if (gainPressedHere && hovered) {
        setLevel(mx01, my01);
    }
    if (edges.leftReleased) {
        gainPressedHere = false;
    }

    if (hovered) {
        for (size_t i = 0; i < gainStrips.size(); i++) {
            if (!gainStrips[i].highlightVisible) {
                continue;
            }
            if (edges.wheelRotV != 0) {
                gainStrips[i].value = GainSlider::clampGain(
                    GainSlider::wheelGainStep(gainStrips[i].value, edges.wheelRotV, gainStrips[i].low,
                        gainStrips[i].high),
                    gainStrips[i].low, gainStrips[i].high);
                gainStrips[i].changed = true;
            }
            if (edges.wheelRotH != 0) {
                gainStrips[i].value = GainSlider::clampGain(
                    GainSlider::wheelGainStep(gainStrips[i].value, edges.wheelRotH, gainStrips[i].low,
                        gainStrips[i].high),
                    gainStrips[i].low, gainStrips[i].high);
                gainStrips[i].changed = true;
            }
        }
    }

    ColorTheme *theme = ThemeMgr::mgr.currentTheme;
    ThemeColor bg{theme->generalBackground.r, theme->generalBackground.g, theme->generalBackground.b};
    ThemeColor level{theme->meterLevel.r, theme->meterLevel.g, theme->meterLevel.b};
    imguiSetClearColor(packClear(bg));

    PanelFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, wsize.x, wsize.y};
    ImU32 bgL = packColor(bg.r, bg.g, bg.b);
    ImU32 bgR = packColor(bg.r * 0.5f, bg.g * 0.5f, bg.b * 0.5f);
    f.dl->AddRectFilledMultiColor(ImVec2(f.px(-1.0f), f.py(1.0f)), ImVec2(f.px(1.0f), f.py(-1.0f)), bgL, bgR, bgR, bgL);

    float fontPx = labelFontSize((float)layout.valH * wsize.y);
    for (size_t i = 0; i < gainStrips.size(); i++) {
        GainStrip &g = gainStrips[i];
        float midX = GainSlider::gainBarMidX(strip, (int)i);
        float x0 = f.px(midX - strip.barWidth);
        float x1 = f.px(midX + strip.barWidth);
        float yTop = f.py(bgH);
        float yBot = f.py(-bgH);
        f.dl->AddRectFilledMultiColor(ImVec2(x0, yTop), ImVec2(x1, yBot), bgL, bgR, bgR, bgL);
        f.dl->AddRect(ImVec2(x0, yTop), ImVec2(x1, yBot), packColor(0.8f, 0.8f, 0.8f), 0.0f, 0, 1.0f);

        float lvNorm = GainSlider::levelNorm(g.value, g.low, g.high);
        float lvTop = f.py(bgH * (-1.0f + 2.0f * lvNorm));
        ImU32 lvEdgeL = packColor(level.r * 0.5f + bg.r, level.g * 0.5f + bg.g, level.b * 0.5f + bg.b);
        ImU32 lvMid =
            packColor(level.r + bg.r * 0.75f, level.g + bg.g * 0.75f, level.b + bg.b * 0.75f);
        ImU32 lvEdgeR = packColor(level.r * 0.5f + bg.r * 0.5f, level.g * 0.5f + bg.g * 0.5f,
            level.b * 0.5f + bg.b * 0.5f);
        emitHBar(f, x0 + 1.0f, x1 - 1.0f, lvTop, yBot - 1.0f, lvEdgeL, lvMid, lvEdgeR);

        if (g.highlightVisible) {
            float hiNorm = GainSlider::levelNorm(g.highlight, g.low, g.high);
            float loNorm = lvNorm < hiNorm ? lvNorm : hiNorm;
            float loTop = f.py(bgH * (-1.0f + 2.0f * loNorm));
            ImU32 hiEdgeL = packColor(0.3f + level.r * 0.5f + bg.r, 0.3f + level.g * 0.5f + bg.g,
                0.3f + level.b * 0.5f + bg.b);
            ImU32 hiMid = packColor(0.65f + level.r + bg.r * 0.75f, 0.65f + level.g + bg.g * 0.75f,
                0.65f + level.b + bg.b * 0.75f);
            ImU32 hiEdgeR = packColor(0.3f + level.r * 0.5f + bg.r * 0.5f, 0.3f + level.g * 0.5f + bg.g * 0.5f,
                0.3f + level.b * 0.5f + bg.b * 0.5f);
            emitHBar(f, x0 + 1.0f, x1 - 1.0f, loTop, yBot - 1.0f, hiEdgeL, hiMid, hiEdgeR);
            if (hiNorm > loNorm) {
                float hiTop = f.py(bgH * (-1.0f + 2.0f * hiNorm));
                ImU32 gxEdgeL = packColor(0.3f + bg.r, 0.3f + bg.g, 0.3f + bg.b);
                ImU32 gxMid = packColor(0.65f + bg.r * 0.75f, 0.65f + bg.g * 0.75f, 0.65f + bg.b * 0.75f);
                ImU32 gxEdgeR =
                    packColor(0.3f + bg.r * 0.5f, 0.3f + bg.g * 0.5f, 0.3f + bg.b * 0.5f);
                emitHBar(f, x0 + 1.0f, x1 - 1.0f, hiTop, loTop, gxEdgeL, gxMid, gxEdgeR);
            }
        }

        float cx = f.px(midX);
        centerText(f, g.name.c_str(), cx, f.py((float)layout.labY), fontPx);
        centerText(f, GainSlider::gainLabelText(g.value).c_str(), cx, f.py((float)layout.valY), fontPx);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);

    if (!gainPressedHere && !helpTip.empty() && hovered) {
        wxGetApp().getAppFrame()->setStatusText(this, helpTip);
    }
}

void GainCanvas::onEnterView() {
    SetCursor(wxCURSOR_CROSS);
#ifdef _WIN32
    if (wxGetApp().getAppFrame()->canFocus()) {
        SetFocus();
    }
#endif
}

void GainCanvas::onLeaveView() {
    SetCursor(wxCURSOR_CROSS);
}
