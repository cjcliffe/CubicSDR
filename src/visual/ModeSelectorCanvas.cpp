// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModeSelectorCanvas.h"

#include <imgui.h>

#include "ui/imgui/imgui_bgfx.h"
#include "ui/imgui/imgui_draw_util.h"
#include "ModeSelector.h"
#include "CubicSDR.h"
#include "AppFrame.h"

#include <chrono>
#include <cmath>
#include <cstring>

ModeSelectorCanvas::ModeSelectorCanvas(wxWindow *parent, const std::string &windowName)
    : BgfxCanvas(parent), viewName(windowName), numChoices(0), currentSelection(-1), toggleMode(false),
      inputChanged(false), padX(4), padY(4), highlightColor(1, 1, 1), highlightOverride(false),
      pulseMode(false), pulsePhase(0), disabled(false), pressed(false), originX(0.0f), lastMx(-FLT_MAX),
      lastMy(-FLT_MAX) {
    SetCursor(wxCURSOR_HAND);
}

ModeSelectorCanvas::~ModeSelectorCanvas() = default;

void ModeSelectorCanvas::frameUpdate() {
    if (pulseMode && currentSelection != -1) {
        long long timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
                               .count();
        pulsePhase = ModeSelector::pulsePhaseAt(timeMs);
        Refresh();
    }
}

void ModeSelectorCanvas::onEnterView() {
    if (!helpTip.empty()) {
        wxGetApp().getAppFrame()->setStatusText(this, helpTip);
    }
    SetCursor(wxCURSOR_HAND);
}

void ModeSelectorCanvas::onLeaveView() {
    SetCursor(wxCURSOR_CROSS);
    pressed = false;
}

void ModeSelectorCanvas::drawView(const BgfxEdges &edges, float mx, float my) {
    wxSize size = GetClientSize();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)size.x, (float)size.y), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin(viewName.c_str(), nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    bool hovered = isMouseInView() && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    lastMx = mx;
    lastMy = my;
    float my01b = 1.0f - (my - origin.y) / wsize.y;
    int yval = ModeSelector::hoveredRow(my01b, numChoices, hovered);

    if (edges.mouseLeft) {
        pressed = false;
    }
    if (edges.leftPressed && hovered) {
        pressed = true;
        originX = mx;
    }
    if (edges.leftReleased && pressed) {
        pressed = false;
        bool isClick = ((mx - originX) / wsize.x) < 2.0f / wsize.y;
        ModeSelector::ReleaseResult result =
            ModeSelector::resolveRelease(currentSelection, yval, toggleMode, isClick, disabled);
        if (result.changed) {
            inputChanged = true;
        }
        currentSelection = result.selection;
        SetCursor(wxCURSOR_HAND);
    }

    ColorTheme *theme = ThemeMgr::mgr.currentTheme;
    imguiSetClearColor(((uint32_t)clampByte(theme->generalBackground.r) << 24)
        | ((uint32_t)clampByte(theme->generalBackground.g) << 16)
        | ((uint32_t)clampByte(theme->generalBackground.b) << 8) | 0xFFu);

    for (int i = 0; i < numChoices; i++) {
        ModeSelector::SelectorCell cell =
            ModeSelector::selectorCell(i, numChoices, (double)wsize.x, (double)wsize.y, padX, padY);
        float x0 = origin.x + (cell.outline[0] + 1.0f) * 0.5f * wsize.x;
        float y0 = origin.y + (1.0f - cell.outline[1]) * 0.5f * wsize.y;
        float x1 = origin.x + (cell.outline[4] + 1.0f) * 0.5f * wsize.x;
        float y1 = origin.y + (1.0f - cell.outline[5]) * 0.5f * wsize.y;
        float rx0 = roundf(x0 < x1 ? x0 : x1);
        float ry0 = roundf(y0 < y1 ? y0 : y1);
        float rx1 = roundf(x0 < x1 ? x1 : x0);
        float ry1 = roundf(y0 < y1 ? y1 : y0);
        float labelY = origin.y + (1.0f - cell.labelY) * 0.5f * wsize.y;
        float labelX = origin.x + wsize.x * 0.5f;
        bool rowHovered = (yval == i && !highlightOverride && !disabled);
        if (rowHovered) {
            ImU32 hc = packColor(theme->buttonHighlight.r, theme->buttonHighlight.g, theme->buttonHighlight.b);
            dl->AddRectFilled(ImVec2(rx0, ry0), ImVec2(rx1, ry1), hc);
            drawCenterText(dl, selections[i].label.c_str(), labelX, labelY, (float)cell.fontSize,
                IM_COL32(0, 0, 0, 255));
        } else {
            RGBA4f hc = theme->button;
            if (highlightOverride) {
                hc = highlightColor;
            }
            if (pulseMode && i == currentSelection) {
                ImU32 off = packColor(hc.r, hc.g, hc.b);
                dl->AddRect(ImVec2(rx0, ry0), ImVec2(rx1, ry1), off, 0.0f, 0, 1.0f);
                drawCenterText(
                    dl, selections[i].label.c_str(), labelX, labelY, (float)cell.fontSize, off);
                float blend = ModeSelector::pulseBlend(pulsePhase);
                ImU32 on = packColor(hc.r, hc.g, hc.b, blend);
                dl->AddRectFilled(ImVec2(rx0, ry0), ImVec2(rx1, ry1), on);
                drawCenterText(dl, selections[i].label.c_str(), labelX, labelY, (float)cell.fontSize,
                    packColor(0.0f, 0.0f, 0.0f, blend));
            } else {
                float alpha = 1.0f;
                if (disabled) {
                    hc = theme->button;
                    alpha = 0.3f;
                }
                bool on = (i == currentSelection);
                ImU32 col = packColor(hc.r, hc.g, hc.b, alpha);
                if (on) {
                    dl->AddRectFilled(ImVec2(rx0, ry0), ImVec2(rx1, ry1), col);
                    drawCenterText(dl, selections[i].label.c_str(), labelX, labelY,
                        (float)cell.fontSize, packColor(0.0f, 0.0f, 0.0f, alpha));
                } else {
                    dl->AddRect(ImVec2(rx0, ry0), ImVec2(rx1, ry1), col, 0.0f, 0, 1.0f);
                    drawCenterText(
                        dl, selections[i].label.c_str(), labelX, labelY, (float)cell.fontSize, col);
                }
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

int ModeSelectorCanvas::getHoveredSelection() {
    wxSize size = GetClientSize();
    if (size.x <= 0 || size.y <= 0) {
        return -1;
    }
    float my01b = 1.0f - lastMy / (float)size.y;
    return ModeSelector::hoveredRow(my01b, numChoices, isMouseInView());
}

void ModeSelectorCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

void ModeSelectorCanvas::addChoice(int value, std::string label) {
    selections.push_back(ModeSelectorMode(value, label));
    numChoices = (int)selections.size();
    Refresh();
}

void ModeSelectorCanvas::addChoice(std::string label) {
    int value = (int)selections.size() + 1;
    addChoice(value, label);
}

void ModeSelectorCanvas::setSelection(const std::string& label) {
    for (size_t i = 0; i < selections.size(); i++) {
        if (selections[i].label == label) {
            currentSelection = (int)i;
            Refresh();
            return;
        }
    }
    currentSelection = -1;
    Refresh();
}

std::string ModeSelectorCanvas::getSelectionLabel() {
    if (currentSelection >= 0 && currentSelection < (int)selections.size()) {
        return selections[currentSelection].label;
    }
    return "";
}

void ModeSelectorCanvas::setSelection(int value) {
    for (size_t i = 0; i < selections.size(); i++) {
        if (selections[i].value == value) {
            currentSelection = (int)i;
            Refresh();
            return;
        }
    }
    currentSelection = -1;
    Refresh();
}

int ModeSelectorCanvas::getSelection() {
    if (currentSelection >= 0 && currentSelection < (int)selections.size()) {
        return selections[currentSelection].value;
    } else {
        return -1;
    }
}

void ModeSelectorCanvas::setToggleMode(bool toggleMode_in) {
    toggleMode = toggleMode_in;
}

bool ModeSelectorCanvas::modeChanged() const {
    return inputChanged;
}

void ModeSelectorCanvas::clearModeChanged() {
    inputChanged = false;
}

void ModeSelectorCanvas::setPadding(float padX_in, float padY_in) {
    padX = padX_in;
    padY = padY_in;
    Refresh();
}

void ModeSelectorCanvas::setHighlightColor(const RGBA4f& hc) {
    highlightColor = hc;
    highlightOverride = true;
    Refresh();
}

void ModeSelectorCanvas::setPulseMode(bool pulseMode_in) {
    pulseMode = pulseMode_in;
}

void ModeSelectorCanvas::setDisabled(bool disabled_in) {
    disabled = disabled_in;
    Refresh();
}

bool ModeSelectorCanvas::isDisabled() const {
    return disabled;
}
