// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Raylib tuner spike: the tuning strip rendered with raylib and driven by the
// tested TunerStep/TunerLayout models plus TuneRange. Device and demodulator
// state is a plain local struct with a synthetic RTLSDR-like range; the wx
// app, GL code, and dialogs are deliberately absent. Colors copy the default
// theme with the same additive shading the GL canvas uses.
// Deliberate deviations from TuningCanvas: no delta-lock state and SPACE has
// no direct-input dialog.

#include "raylib.h"

#include "TunerLayout.h"
#include "TunerStep.h"
#include "TuneRange.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

const long long CHANNELIZER_MAX = 500000;

const float FREQ_DP = -1.0f;
const float FREQ_W = (1.0f / 3.0f) * 2.0f;
const int FREQ_COUNT = 11;
const float BW_DP = -1.0f + (2.25f / 3.0f);
const float BW_W = (1.0f / 4.0f) * 2.0f;
const int BW_COUNT = 7;
const float CENTER_DP = -1.0f + (2.0f / 3.0f) * 2.0f;
const float CENTER_W = (1.0f / 3.0f) * 2.0f;
const int CENTER_COUNT = 11;

struct RGB {
    float r, g, b;
};

const RGB THEME_BG = { 0.1f, 0.1f, 0.1f };
const RGB THEME_MID = { 0.2f, 0.2f, 0.9f };
const RGB THEME_DARK = { 0.0f, 0.0f, 0.35f };
const RGB THEME_UP = { 1.0f, 139.0f / 255.0f, 96.0f / 255.0f };
const RGB THEME_DOWN = { 148.0f / 255.0f, 148.0f / 255.0f, 1.0f };

const long long MIN_BW = 500;

Color toColor(float r, float g, float b) {
    if (r < 0.0f) {
        r = 0.0f;
    } else if (r > 1.0f) {
        r = 1.0f;
    }
    if (g < 0.0f) {
        g = 0.0f;
    } else if (g > 1.0f) {
        g = 1.0f;
    }
    if (b < 0.0f) {
        b = 0.0f;
    } else if (b > 1.0f) {
        b = 1.0f;
    }
    Color color;
    color.r = (unsigned char)(r * 255.0f);
    color.g = (unsigned char)(g * 255.0f);
    color.b = (unsigned char)(b * 255.0f);
    color.a = 255;
    return color;
}

Color bandShade(RGB band, float k) {
    return toColor(THEME_BG.r + k * band.r, THEME_BG.g + k * band.g, THEME_BG.b + k * band.b);
}

Color hoverShade(RGB band, RGB hover, float k) {
    return toColor(THEME_BG.r + k * band.r + k * hover.r, THEME_BG.g + k * band.g + k * hover.g,
        THEME_BG.b + k * band.b + k * hover.b);
}

bool isMidBand(int index, int count) {
    if (count == 7) {
        return index <= 3 || index == 7;
    }
    return ((index - 1) / 3) % 2 == 0;
}

const Color GRID = { 165, 165, 165, 64 };
const Color DIGIT = { 255, 255, 255, 255 };
const Color SNAPBOX = { 255, 0, 0, 255 };
const Color OVERLAY_BG = { 0, 0, 0, 160 };
const Color OVERLAY_FG = { 160, 160, 160, 255 };

enum HoverState { HOVER_NONE, HOVER_FREQ, HOVER_BW, HOVER_PPM, HOVER_CENTER };
enum TuningDirection { DIR_DOWN, DIR_UP };

struct AppState {
    long long freq = 146520000;
    long long bw = 200000;
    long long center = 146000000;
    int ppm = 0;
    int snap = 1;
    bool halfBand = false;
    long long sampleRate = 2000000;
    long long deviceMin = 500000;
    long long deviceMax = 1750000000;
    long long offset = 0;
};

int ndcToPx(float ndc, int w) {
    return (int)((ndc + 1.0f) * 0.5f * (float)w);
}

int ndcToPy(float ndc, int h) {
    return (int)((1.0f - ndc) * 0.5f * (float)h);
}

void drawBar(const std::vector<float> &quad, int w, int h, Color outer, Color inner, bool isTop) {
    if (quad.size() < 8) {
        return;
    }
    float x0 = quad[0], y0 = quad[1], x1 = quad[0], y1 = quad[1];
    for (size_t i = 2; i + 1 < quad.size(); i += 2) {
        if (quad[i] < x0) {
            x0 = quad[i];
        }
        if (quad[i] > x1) {
            x1 = quad[i];
        }
        if (quad[i + 1] < y0) {
            y0 = quad[i + 1];
        }
        if (quad[i + 1] > y1) {
            y1 = quad[i + 1];
        }
    }
    int px = ndcToPx(x0, w);
    int py = ndcToPy(y1, h);
    int pw = ndcToPx(x1, w) - px;
    int ph = ndcToPy(y0, h) - py;
    if (isTop) {
        DrawRectangleGradientV(px, py, pw, ph, outer, inner);
    } else {
        DrawRectangleGradientV(px, py, pw, ph, inner, outer);
    }
}

void drawBars(int start, int end, int count, float dp, float dw, int w, int h, RGB band) {
    TunerLayout::TunerBars bars = TunerLayout::tunerBarQuads(start, end, count, dp, dw, true, true);
    drawBar(bars.top, w, h, bandShade(band, 0.5f), bandShade(band, 1.0f), true);
    drawBar(bars.bottom, w, h, bandShade(band, 0.5f), bandShade(band, 1.0f), false);
}

void drawDigits(long long value, int count, float dp, float dw, int w, int h, int fontSize) {
    std::string chars = std::to_string(value);
    std::vector<TunerLayout::TunerChar> layout = TunerLayout::tunerChars(count, (int)chars.length(), dp, dw);
    for (size_t i = 0; i < layout.size(); i++) {
        std::string glyph = chars.substr((size_t)layout[i].charIndex, 1);
        int tw = MeasureText(glyph.c_str(), fontSize);
        DrawText(glyph.c_str(), ndcToPx(layout[i].x, w) - tw / 2, h / 2 - fontSize / 2, fontSize, DIGIT);
    }
    std::vector<float> grid = TunerLayout::tunerGridVerts(count, dp, dw);
    for (size_t i = 0; i + 3 < grid.size(); i += 4) {
        int px = ndcToPx(grid[i], w);
        DrawLine(px, 0, px, h, GRID);
    }
}

void drawHover(int index, int count, float dp, float dw, int w, int h, bool top, bool bottom) {
    TunerLayout::TunerBars bars = TunerLayout::tunerBarQuads(index, index, count, dp, dw, top, bottom);
    RGB band = isMidBand(index, count) ? THEME_MID : THEME_DARK;
    RGB hover = top ? THEME_UP : THEME_DOWN;
    drawBar(bars.top, w, h, hoverShade(band, hover, 0.5f), hoverShade(band, hover, 1.0f), true);
    drawBar(bars.bottom, w, h, hoverShade(band, hover, 0.5f), hoverShade(band, hover, 1.0f), false);
}

void drawSnapBox(int snap, int w, int h) {
    std::vector<float> box = TunerLayout::tunerDigitBox((int)log10((double)snap), FREQ_COUNT, FREQ_DP, FREQ_W, (float)h);
    for (size_t i = 0; i + 3 < box.size(); i += 2) {
        DrawLine(ndcToPx(box[i], w), ndcToPy(box[i + 1], h), ndcToPx(box[i + 2], w), ndcToPy(box[i + 3], h), SNAPBOX);
    }
}

void stepTuner(HoverState state, TuningDirection dir, int digit, bool preventCarry, bool zeroOut, bool bottom, AppState &app) {
    TunerStep::StepInput step;
    step.digit = digit;
    step.up = (dir == DIR_UP);
    step.preventCarry = preventCarry;
    step.zeroOut = zeroOut;
    step.bottom = bottom;
    if (state == HOVER_FREQ) {
        long long before = app.freq;
        step.value = before;
        app.freq = TunerStep::stepDigit(step);
        app.freq = pinDemodCenter(app.freq, app.center, app.sampleRate);
        long long diff = app.center > before ? app.center - before : before - app.center;
        if (app.sampleRate / 2 < diff) {
            app.center = clampTuneFrequency(app.freq, app.offset, deviceTuneFloor(app.deviceMin, app.sampleRate), app.deviceMax);
        }
    } else if (state == HOVER_BW) {
        step.value = app.bw;
        step.doubleAmount = app.halfBand;
        long long nbw = TunerStep::stepDigit(step);
        if (nbw > CHANNELIZER_MAX) {
            nbw = CHANNELIZER_MAX;
        }
        if (nbw < MIN_BW) {
            nbw = MIN_BW;
        }
        if (nbw > app.sampleRate) {
            nbw = app.sampleRate;
        }
        app.bw = nbw;
    } else if (state == HOVER_CENTER) {
        step.value = app.center;
        app.center = clampTuneFrequency(TunerStep::stepDigit(step), app.offset,
            deviceTuneFloor(app.deviceMin, app.sampleRate), app.deviceMax);
    } else if (state == HOVER_PPM) {
        step.value = app.ppm;
        int ppm = (int)TunerStep::stepDigit(step);
        if (ppm > 2000) {
            ppm = 2000;
        }
        if (ppm < -2000) {
            ppm = -2000;
        }
        app.ppm = ppm;
    }
}

const char *stateName(HoverState state) {
    switch (state) {
    case HOVER_FREQ:
        return "FREQ";
    case HOVER_BW:
        return "BW";
    case HOVER_PPM:
        return "PPM";
    case HOVER_CENTER:
        return "CENTER";
    default:
        return "NONE";
    }
}

} // namespace

int main() {
    AppState app;
    HoverState hoverState = HOVER_NONE, downState = HOVER_NONE, rightDownState = HOVER_NONE;
    int hoverIndex = 0, downIndex = 0, rightDownIndex = 0;
    bool leftDown = false, dragging = false;
    float dragAccum = 0.0f, originX = 0.0f;
    int wheelAccumV = 0, wheelAccumH = 0;
    float wheelFracV = 0.0f, wheelFracH = 0.0f;

    InitWindow(900, 160, "CubicSDR tuner spike (raylib, no wx)");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        Vector2 mouse = GetMousePosition();
        float mx = mouse.x / (float)w;
        float my = 1.0f - mouse.y / (float)h;
        bool top = my >= 0.5f;
        bool bottom = my <= 0.5f;
        bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        bool altDown = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

        hoverIndex = 0;
        hoverState = HOVER_NONE;
        int index = TunerLayout::digitIndexAt(mx, FREQ_COUNT, FREQ_DP, FREQ_W);
        if (index > 0) {
            hoverIndex = index;
            hoverState = altDown ? HOVER_PPM : HOVER_FREQ;
        }
        if (index == 0) {
            index = TunerLayout::digitIndexAt(mx, BW_COUNT, BW_DP, BW_W);
            if (index > 0) {
                hoverIndex = index;
                hoverState = HOVER_BW;
            }
        }
        if (index == 0) {
            index = TunerLayout::digitIndexAt(mx, CENTER_COUNT, CENTER_DP, CENTER_W);
            if (index > 0) {
                hoverIndex = index;
                hoverState = HOVER_CENTER;
            }
        }

        if (IsKeyPressed(KEY_H)) {
            app.halfBand = !app.halfBand;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            leftDown = true;
            originX = mx;
            dragAccum = 0.0f;
            downIndex = hoverIndex;
            downState = hoverState;
            dragging = false;
        }
        if (leftDown) {
            if (downState != HOVER_NONE) {
                dragAccum += 5.0f * (mx - originX);
                TunerStep::DragSteps drag = TunerStep::dragSteps(dragAccum);
                dragAccum = drag.rest;
                for (int i = 0; i < drag.up; i++) {
                    stepTuner(downState, DIR_UP, downIndex - 1, shiftDown, false, bottom, app);
                    dragging = true;
                }
                for (int i = 0; i < drag.down; i++) {
                    stepTuner(downState, DIR_DOWN, downIndex - 1, shiftDown, false, bottom, app);
                    dragging = true;
                }
            } else {
                dragAccum = 0.0f;
                dragging = false;
            }
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (hoverState != HOVER_NONE && !dragging && downState == hoverState && downIndex == hoverIndex) {
                stepTuner(hoverState, top ? DIR_UP : DIR_DOWN, hoverIndex - 1, shiftDown, false, bottom, app);
            }
            leftDown = false;
            dragging = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            rightDownIndex = hoverIndex;
            rightDownState = hoverState;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            if (shiftDown) {
                if (hoverState != HOVER_NONE && !dragging && rightDownState == hoverState && rightDownIndex == hoverIndex) {
                    stepTuner(hoverState, top ? DIR_UP : DIR_DOWN, hoverIndex - 1, false, true, bottom, app);
                }
            } else if (hoverState == HOVER_FREQ) {
                if (hoverIndex == 1) {
                    app.snap = 1;
                } else if (hoverIndex > 1 && hoverIndex < 8) {
                    int exp = (int)pow(10, hoverIndex - 1);
                    app.snap = (app.snap == exp) ? 1 : exp;
                }
            }
        }

        if (hoverState != HOVER_NONE && !leftDown && hoverIndex > 0) {
            Vector2 wheel = GetMouseWheelMoveV();
            wheelFracV += wheel.y * 120.0f;
            wheelFracH += wheel.x * 120.0f;
            int rotV = (int)wheelFracV;
            int rotH = (int)wheelFracH;
            wheelFracV -= (float)rotV;
            wheelFracH -= (float)rotH;
            TunerStep::WheelSteps wsv = TunerStep::wheelSteps(wheelAccumV, rotV, 120);
            wheelAccumV = wsv.rest;
            for (int i = 0, n = wsv.steps > 0 ? wsv.steps : -wsv.steps; i < n; i++) {
                stepTuner(hoverState, (wsv.steps > 0) ? DIR_UP : DIR_DOWN, hoverIndex - 1, shiftDown, false, bottom, app);
            }
            TunerStep::WheelSteps wsh = TunerStep::wheelSteps(wheelAccumH, rotH, 120);
            wheelAccumH = wsh.rest;
            for (int i = 0, n = wsh.steps > 0 ? wsh.steps : -wsh.steps; i < n; i++) {
                stepTuner(hoverState, (wsh.steps < 0) ? DIR_UP : DIR_DOWN, hoverIndex - 1, shiftDown, false, bottom, app);
            }
        }

        BeginDrawing();
        ClearBackground(toColor(THEME_BG.r, THEME_BG.g, THEME_BG.b));

        drawBars(1, 3, FREQ_COUNT, FREQ_DP, FREQ_W, w, h, THEME_MID);
        drawBars(4, 6, FREQ_COUNT, FREQ_DP, FREQ_W, w, h, THEME_DARK);
        drawBars(7, 9, FREQ_COUNT, FREQ_DP, FREQ_W, w, h, THEME_MID);
        drawBars(10, 11, FREQ_COUNT, FREQ_DP, FREQ_W, w, h, THEME_DARK);
        drawBars(1, 3, BW_COUNT, BW_DP, BW_W, w, h, THEME_MID);
        drawBars(4, 6, BW_COUNT, BW_DP, BW_W, w, h, THEME_DARK);
        drawBars(7, 7, BW_COUNT, BW_DP, BW_W, w, h, THEME_MID);
        drawBars(1, 3, CENTER_COUNT, CENTER_DP, CENTER_W, w, h, THEME_MID);
        drawBars(4, 6, CENTER_COUNT, CENTER_DP, CENTER_W, w, h, THEME_DARK);
        drawBars(7, 9, CENTER_COUNT, CENTER_DP, CENTER_W, w, h, THEME_MID);
        drawBars(10, 11, CENTER_COUNT, CENTER_DP, CENTER_W, w, h, THEME_DARK);

        if (hoverIndex > 0 && !leftDown) {
            if (hoverState == HOVER_FREQ || hoverState == HOVER_PPM) {
                drawHover(hoverIndex, FREQ_COUNT, FREQ_DP, FREQ_W, w, h, top, bottom);
            } else if (hoverState == HOVER_BW) {
                drawHover(hoverIndex, BW_COUNT, BW_DP, BW_W, w, h, top, bottom);
            } else if (hoverState == HOVER_CENTER) {
                drawHover(hoverIndex, CENTER_COUNT, CENTER_DP, CENTER_W, w, h, top, bottom);
            }
        }

        int fontSize = TunerLayout::tunerFontSize((float)w, (float)h);
        if (altDown) {
            drawDigits(app.ppm, FREQ_COUNT, FREQ_DP, FREQ_W, w, h, fontSize);
        } else {
            drawDigits(app.freq, FREQ_COUNT, FREQ_DP, FREQ_W, w, h, fontSize);
            if (app.snap != 1) {
                drawSnapBox(app.snap, w, h);
            }
        }
        drawDigits(app.halfBand ? (app.bw / 2) : app.bw, BW_COUNT, BW_DP, BW_W, w, h, fontSize);
        drawDigits(app.center, CENTER_COUNT, CENTER_DP, CENTER_W, w, h, fontSize);

        char status[256];
        snprintf(status, sizeof(status), "%s idx=%d freq=%lld bw=%lld center=%lld ppm=%d snap=%d halfBand=%d",
            stateName(hoverState), hoverIndex, app.freq, app.bw, app.center, app.ppm, app.snap, app.halfBand ? 1 : 0);
        DrawRectangle(4, h - 20, MeasureText(status, 10) + 8, 16, OVERLAY_BG);
        DrawText(status, 8, h - 18, 10, OVERLAY_FG);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
