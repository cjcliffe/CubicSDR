// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Raylib mode-selector spike: the vertical choice list rendered with raylib
// and driven by the tested ModeSelector model. Choices mirror the modem
// selector; keys flip the widget modes the single-button selectors use.
// Deliberate deviations from ModeSelectorCanvas: the press origin freezes the
// hover instead of warping the OS cursor (same resulting state on Windows),
// and there is no status-tip plumbing.

#include "raylib.h"

#include "ModeSelector.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace {

const Color THEME_BG = { 26, 26, 26, 255 };
const Color THEME_BUTTON = { 166, 166, 166, 255 };
const Color THEME_HIGHLIGHT = { 255, 255, 0, 255 };
const Color THEME_OVERRIDE = { 255, 255, 255, 255 };
const Color TEXT_ON = { 0, 0, 0, 255 };
const Color OVERLAY_BG = { 0, 0, 0, 200 };
const Color OVERLAY_FG = { 160, 160, 160, 255 };

struct SelectorState {
    std::vector<std::string> labels = {"FM", "FMS", "NBFM", "AM", "CW", "LSB", "USB", "DSB", "I/Q"};
    int selection = -1;
    bool toggleMode = false;
    bool pulseMode = false;
    bool disabled = false;
    bool highlightOverride = false;
    float padX = 4.0f;
    float padY = 4.0f;
    float pulsePhase = 0.0f;
    bool inputChanged = false;
};

int ndcToPx(float ndc, int w) {
    return (int)((ndc + 1.0f) * 0.5f * (float)w);
}

int ndcToPy(float ndc, int h) {
    return (int)((1.0f - ndc) * 0.5f * (float)h);
}

void drawSelector(const std::string &label, int c, int cMax, bool on, Color color, float alpha, float padX, float padY, int w, int h) {
    ModeSelector::SelectorCell cell = ModeSelector::selectorCell(c, cMax, (float)w, (float)h, padX, padY);
    Color faded = Fade(color, alpha);
    int x0 = ndcToPx(cell.outline[0], w);
    int x1 = ndcToPx(cell.outline[2], w);
    int yTop = ndcToPy(cell.outline[5], h);
    int yBottom = ndcToPy(cell.outline[1], h);
    if (on) {
        DrawRectangle(x0, yTop, x1 - x0, yBottom - yTop, faded);
    } else {
        Rectangle box = {(float)x0, (float)yTop, (float)(x1 - x0), (float)(yBottom - yTop)};
        DrawRectangleLinesEx(box, 1.0f, faded);
    }
    Color textColor = on ? Fade(TEXT_ON, alpha) : faded;
    int tw = MeasureText(label.c_str(), cell.fontSize);
    DrawText(label.c_str(), (w - tw) / 2, ndcToPy(cell.labelY, h) - cell.fontSize / 2, cell.fontSize, textColor);
}

} // namespace

int main() {
    SelectorState sel;
    bool leftDown = false;
    float originX = 0.0f, originY = 0.0f;

    InitWindow(220, 360, "CubicSDR mode selector spike (raylib, no wx)");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        Vector2 mouse = GetMousePosition();
        float mx = mouse.x / (float)w;
        float my = 1.0f - mouse.y / (float)h;
        bool inView = mouse.x >= 0 && mouse.x < (float)w && mouse.y >= 0 && mouse.y < (float)h;
        if (leftDown) {
            mx = originX;
            my = originY;
            inView = true;
        }
        int numChoices = (int)sel.labels.size();
        int hovered = ModeSelector::hoveredRow(my, numChoices, inView);

        if (IsKeyPressed(KEY_T)) {
            sel.toggleMode = !sel.toggleMode;
        }
        if (IsKeyPressed(KEY_P)) {
            sel.pulseMode = !sel.pulseMode;
        }
        if (IsKeyPressed(KEY_D)) {
            sel.disabled = !sel.disabled;
        }
        if (IsKeyPressed(KEY_H)) {
            sel.highlightOverride = !sel.highlightOverride;
        }
        if (IsKeyPressed(KEY_C)) {
            sel.inputChanged = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            leftDown = true;
            originX = mx;
            originY = my;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            leftDown = false;
            if (!sel.disabled) {
                bool isClick = (mx - originX) < 2.0f / (float)h;
                ModeSelector::ReleaseResult result = ModeSelector::resolveRelease(sel.selection, hovered, sel.toggleMode, isClick,
                    sel.disabled);
                if (result.changed) {
                    sel.inputChanged = true;
                }
                sel.selection = result.selection;
            }
        }

        float blend = 0.0f;
        if (sel.pulseMode && sel.selection != -1) {
            long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            sel.pulsePhase = ModeSelector::pulsePhaseAt(ms);
            blend = ModeSelector::pulseBlend(sel.pulsePhase);
        }

        BeginDrawing();
        ClearBackground(THEME_BG);
        for (int i = 0; i < numChoices; i++) {
            if (hovered == i && !sel.highlightOverride && !sel.disabled) {
                drawSelector(sel.labels[i], i, numChoices, true, THEME_HIGHLIGHT, 1.0f, sel.padX, sel.padY, w, h);
            } else {
                Color hc = sel.highlightOverride ? THEME_OVERRIDE : THEME_BUTTON;
                if (sel.pulseMode && i == sel.selection) {
                    drawSelector(sel.labels[i], i, numChoices, false, hc, 1.0f, sel.padX, sel.padY, w, h);
                    drawSelector(sel.labels[i], i, numChoices, true, hc, blend, sel.padX, sel.padY, w, h);
                } else {
                    float alpha = 1.0f;
                    if (sel.disabled) {
                        hc = THEME_BUTTON;
                        alpha = 0.3f;
                    }
                    drawSelector(sel.labels[i], i, numChoices, i == sel.selection, hc, alpha, sel.padX, sel.padY, w, h);
                }
            }
        }

        char status[256];
        snprintf(status, sizeof(status), "sel=%d hov=%d tgl=%d pls=%d dis=%d ovr=%d chg=%d bl=%.2f", sel.selection, hovered,
            sel.toggleMode ? 1 : 0, sel.pulseMode ? 1 : 0, sel.disabled ? 1 : 0, sel.highlightOverride ? 1 : 0,
            sel.inputChanged ? 1 : 0, blend);
        const char *keys = "T toggle P pulse D disabled H override C clear";
        int sw = MeasureText(status, 10);
        int kw = MeasureText(keys, 10);
        int bw = sw > kw ? sw : kw;
        DrawRectangle(4, h - 38, bw + 8, 34, OVERLAY_BG);
        DrawText(status, 8, h - 36, 10, OVERLAY_FG);
        DrawText(keys, 8, h - 22, 10, OVERLAY_FG);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
