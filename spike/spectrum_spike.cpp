// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Raylib spectrum spike: the main spectrum view rendered with raylib and
// driven by the tested SpectrumTicks model, fed by a synthetic carrier plus
// noise feed in place of the visual processor. Pan, visual gain, dB labels,
// and peak hold follow the canvas code.
// Deliberate deviations: no demodulator markers or hover ghost (they need the
// demodulator manager), peak hold is a local max with manual reseed instead
// of processor state, wheel zoom belongs to the waterfall contract, the font
// scale is fixed at 1, dB chips are flat approximations of the gradient
// panels, and lines are aliased.

#include "raylib.h"

#include "SpectrumTicks.h"
#include "TuneRange.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

const int FFT_SIZE = 2048;
const int NUM_POINTS = 1024;
const double POINT_FLOOR = 3.0;
const double POINT_CEIL = 100.0;
const double DB_OFS = 0.0;

struct RGB {
    float r, g, b;
};

const RGB FFT_BG = { 0.1f, 0.1f, 0.1f };
const RGB FFT_FILL = { 0.15f, 0.35f, 0.85f };
const float FFT_FILL_A = 0.3f;
const RGB FFT_LINE = { 0.9f, 0.9f, 1.0f };
const RGB FREQ_LINE = { 1.0f, 1.0f, 1.0f };

struct Carrier {
    long long freqHz;
    double amp;
    double widthHz;
};

struct AppState {
    long long center = 146000000;
    long long sampleRate = 2000000;
    long long deviceMin = 500000;
    long long deviceMax = 1750000000;
    long long offset = 0;
    float scaleFactor = 1.0f;
    bool resetScale = false;
    bool showDb = true;
    bool peakHold = false;
    std::vector<Carrier> carriers = { {146520000, 85.0, 25000.0}, {145700000, 45.0, 12000.0},
        {146900000, 20.0, 60000.0}, {145200000, 10.0, 8000.0} };
};

Color toColor(float r, float g, float b, float a) {
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
    if (a < 0.0f) {
        a = 0.0f;
    } else if (a > 1.0f) {
        a = 1.0f;
    }
    Color color;
    color.r = (unsigned char)(r * 255.0f);
    color.g = (unsigned char)(g * 255.0f);
    color.b = (unsigned char)(b * 255.0f);
    color.a = (unsigned char)(a * 255.0f);
    return color;
}

int ndcToPx(float ndc, int w) {
    return (int)((ndc + 1.0f) * 0.5f * (float)w);
}

int ndcToPy(float ndc, int h) {
    return (int)((1.0f - ndc) * 0.5f * (float)h);
}

float pointToNdcY(float y) {
    return -0.75f + 1.5f * y;
}

double synthMag(long long f, const AppState &app) {
    double mag = 4.0 + (double)(std::rand() % 1000) / 1000.0 * 2.5;
    for (size_t i = 0; i < app.carriers.size(); i++) {
        double d = ((double)f - (double)app.carriers[i].freqHz) / app.carriers[i].widthHz;
        mag += app.carriers[i].amp * std::exp(-d * d);
    }
    return mag;
}

void drawGrid(double floorValue, double ceilValue, int w, int h) {
    SpectrumTicks::GridGeometry grid = SpectrumTicks::gridGeometry((float)floorValue, (float)ceilValue);
    for (size_t i = 0; i < grid.bands.size(); i++) {
        const std::vector<float> &verts = grid.bands[i].verts;
        for (size_t v = 0; v + 3 < verts.size(); v += 4) {
            float p = verts[v + 1];
            float ndcY = pointToNdcY(p);
            float bg = FFT_BG.r + (FFT_BG.r * 2.0f - FFT_BG.r) * ((1.0f - ndcY) / 2.0f);
            float c = bg + (float)grid.bands[i].alpha * 0.12f;
            int y = ndcToPy(ndcY, h);
            DrawLine(ndcToPx(-1.0f + 2.0f * verts[v], w), y, ndcToPx(-1.0f + 2.0f * verts[v + 2], w), y,
                toColor(c, c, c, 1.0f));
        }
    }
}

void drawTrace(const std::vector<float> &points, int w, int h) {
    std::vector<float> fill = SpectrumTicks::fillStrip(points);
    std::vector<Vector2> strip(fill.size() / 2);
    for (size_t i = 0; i < strip.size(); i++) {
        strip[i].x = (float)ndcToPx(-1.0f + 2.0f * fill[i * 2], w);
        strip[i].y = (float)ndcToPy(pointToNdcY(fill[i * 2 + 1]), h);
    }
    DrawTriangleStrip(&strip[0], (int)strip.size(), toColor(FFT_FILL.r, FFT_FILL.g, FFT_FILL.b, FFT_FILL_A));

    int skirtTop = ndcToPy(pointToNdcY(0.0f), h);
    DrawRectangleGradientV(0, skirtTop, w, h - skirtTop, toColor(FFT_FILL.r, FFT_FILL.g, FFT_FILL.b, FFT_FILL_A),
        toColor(FFT_BG.r, FFT_BG.g, FFT_BG.b, FFT_FILL_A));

    std::vector<Vector2> line(points.size() / 2);
    for (size_t i = 0; i < line.size(); i++) {
        line[i].x = (float)ndcToPx(-1.0f + 2.0f * points[i * 2], w);
        line[i].y = (float)ndcToPy(pointToNdcY(points[i * 2 + 1]), h);
    }
    DrawLineStrip(&line[0], (int)line.size(), toColor(FFT_LINE.r, FFT_LINE.g, FFT_LINE.b, 1.0f));
}

void drawAxis(long long freqHz, long long bandwidthHz, int w, int h) {
    SpectrumTicks::AxisGeometry axis = SpectrumTicks::axisGeometry(freqHz, bandwidthHz, (double)w, (double)h, 1.0);
    Color major = toColor(FREQ_LINE.r, FREQ_LINE.g, FREQ_LINE.b, 1.0f);
    Color minor = toColor(FREQ_LINE.r * 0.65f, FREQ_LINE.g * 0.65f, FREQ_LINE.b * 0.65f, 1.0f);
    for (size_t i = 0; i < axis.ticks.size(); i++) {
        int x = ndcToPx((float)axis.ticks[i].x, w);
        Vector2 top = {(float)x, (float)ndcToPy(1.0f, h)};
        Vector2 base = {(float)x, (float)ndcToPy((float)axis.tickBase, h)};
        if (axis.ticks[i].major) {
            DrawLineEx(top, base, 4.0f, major);
        } else {
            DrawLineEx(top, base, 1.0f, minor);
        }
        int tw = MeasureText(axis.ticks[i].label.c_str(), axis.fontSize);
        DrawText(axis.ticks[i].label.c_str(), x - tw / 2, ndcToPy((float)axis.hPos, h) - axis.fontSize / 2, axis.fontSize,
            toColor(1.0f, 1.0f, 1.0f, 1.0f));
    }
}

void drawDbChip(const std::string &text, int y) {
    int tw = MeasureText(text.c_str(), 12);
    int chipW = tw + 16;
    DrawRectangleGradientH(4, y, chipW, 16, toColor(0.2f, 0.2f, 0.2f, 1.0f), toColor(0.2f, 0.2f, 0.2f, 0.0f));
    DrawText(text.c_str(), 4 + chipW - 8 - tw, y + 2, 12, toColor(1.0f, 1.0f, 1.0f, 1.0f));
}

} // namespace

int main() {
    std::srand(42);
    AppState app;
    std::vector<float> points(NUM_POINTS * 2);
    std::vector<double> mags(NUM_POINTS);
    std::vector<double> holdMag;
    bool leftDown = false, rightDown = false;
    bool reseedPeak = false;
    float lastMx = 0.0f, lastMy = 0.0f, originDeltaY = 0.0f;

    InitWindow(900, 420, "CubicSDR spectrum spike (raylib, no wx)");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        Vector2 mouse = GetMousePosition();
        float mx = mouse.x / (float)w;
        float my = 1.0f - mouse.y / (float)h;

        if (IsKeyPressed(KEY_B)) {
            app.showDb = !app.showDb;
        }
        if (IsKeyPressed(KEY_P)) {
            app.peakHold = !app.peakHold;
            reseedPeak = true;
        }
        if (IsKeyPressed(KEY_R)) {
            reseedPeak = true;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            leftDown = true;
            lastMx = mx;
        }
        if (leftDown) {
            int freqChange = (int)((mx - lastMx) * (float)app.sampleRate);
            if (freqChange != 0) {
                app.center = clampTuneFrequency(app.center - freqChange, app.offset,
                    deviceTuneFloor(app.deviceMin, app.sampleRate), app.deviceMax);
            }
            lastMx = mx;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            leftDown = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            rightDown = true;
            lastMy = my;
            originDeltaY = 0.0f;
        }
        if (rightDown) {
            float dy = my - lastMy;
            lastMy = my;
            originDeltaY += dy;
            app.scaleFactor += dy * 2.0f;
            if (app.scaleFactor < 0.25f) {
                app.scaleFactor = 0.25f;
            }
            if (app.scaleFactor > 10.0f) {
                app.scaleFactor = 10.0f;
            }
            app.resetScale = false;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            rightDown = false;
            if (originDeltaY == 0.0f) {
                app.resetScale = true;
            }
        }
        if (app.resetScale) {
            app.scaleFactor += (1.0f - app.scaleFactor) * 0.05f;
            if (std::fabs(app.scaleFactor - 1.0f) < 0.01f) {
                app.scaleFactor = 1.0f;
                app.resetScale = false;
            }
        }

        double yDen = std::log10((POINT_CEIL + 0.25) - (POINT_FLOOR - 0.75));
        for (int i = 0; i < NUM_POINTS; i++) {
            long long f = app.center - app.sampleRate / 2 + (long long)((double)i / (double)NUM_POINTS * (double)app.sampleRate);
            mags[i] = synthMag(f, app);
            double norm = std::log10(mags[i] + 0.25 - (POINT_FLOOR - 0.75)) / yDen;
            points[i * 2] = (float)i / (float)NUM_POINTS;
            points[i * 2 + 1] = (float)(norm * app.scaleFactor);
        }
        if (reseedPeak) {
            holdMag = mags;
            reseedPeak = false;
        }
        if (app.peakHold && holdMag.size() == (size_t)NUM_POINTS) {
            for (int i = 0; i < NUM_POINTS; i++) {
                if (mags[i] > holdMag[i]) {
                    holdMag[i] = mags[i];
                }
            }
        }

        BeginDrawing();
        DrawRectangleGradientV(0, 0, w, h, toColor(FFT_BG.r, FFT_BG.g, FFT_BG.b, 1.0f),
            toColor(FFT_BG.r * 2.0f, FFT_BG.g * 2.0f, FFT_BG.b * 2.0f, 1.0f));
        drawGrid(POINT_FLOOR, POINT_CEIL / app.scaleFactor, w, h);
        drawTrace(points, w, h);
        if (app.peakHold && holdMag.size() == (size_t)NUM_POINTS) {
            std::vector<Vector2> peak(NUM_POINTS);
            for (int i = 0; i < NUM_POINTS; i++) {
                double norm = std::log10(holdMag[i] + 0.25 - (POINT_FLOOR - 0.75)) / yDen;
                peak[i].x = (float)ndcToPx(-1.0f + 2.0f * points[i * 2], w);
                peak[i].y = (float)ndcToPy(pointToNdcY((float)(norm * app.scaleFactor)), h);
            }
            DrawLineStrip(&peak[0], NUM_POINTS, toColor(0.0f, 1.0f, 0.0f, 0.5f));
        }
        drawAxis(app.center, app.sampleRate, w, h);
        if (app.showDb) {
            SpectrumTicks::DbLabels db = SpectrumTicks::dbLabels(POINT_CEIL / app.scaleFactor, POINT_FLOOR, FFT_SIZE, DB_OFS);
            drawDbChip(db.ceil, 4);
            drawDbChip(db.floor, h - 20);
        }

        char status[256];
        snprintf(status, sizeof(status), "center=%lld span=%lld sf=%.2f dB=%d peak=%d", app.center, app.sampleRate,
            app.scaleFactor, app.showDb ? 1 : 0, app.peakHold ? 1 : 0);
        const char *keys = "drag pan  R-drag gain  R-click reset  B dB  P peak  R reseed";
        DrawRectangle(4, h - 38, MeasureText(keys, 10) + 8, 34, toColor(0.0f, 0.0f, 0.0f, 0.75f));
        DrawText(status, 8, h - 36, 10, toColor(0.63f, 0.63f, 0.63f, 1.0f));
        DrawText(keys, 8, h - 22, 10, toColor(0.63f, 0.63f, 0.63f, 1.0f));

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
