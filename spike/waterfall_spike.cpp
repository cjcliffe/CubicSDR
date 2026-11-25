// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

// Raylib waterfall spike: the main waterfall view rendered with raylib and
// driven by the tested WaterfallHover, WaterfallDrag, and DemodMarkers models
// plus the headless WaterfallPacer and WaterfallRing, fed by a synthetic
// carrier plus noise feed. Demodulators are fake local state; the palette is
// the exact theme turbo gradient.
// Deliberate deviations: no squelch cues, delta lock, or dialogs; peak-hold
// reseeds with the gain reset are skipped; the spectrum canvas sync is absent;
// the press origin freezes the hover instead of warping the OS cursor (same
// resulting state on Windows); LPS, snap, and delete are keys instead of
// meter, tuner, and AppFrame behavior.

#include "raylib.h"

// raylib defines PI as a macro; the view models declare it as a typed
// constant instead.
#undef PI

#include "WaterfallDrag.h"
#include "WaterfallHover.h"
#include "WaterfallPacer.h"
#include "WaterfallRing.h"
#include "TuneRange.h"
#include "visual/ColorTheme.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include <vector>

namespace {

const unsigned int FFT_SIZE = 2048;
const int NUM_LINES = 512;
const double POINT_FLOOR = 3.0;
const double POINT_CEIL = 100.0;
const long long MIN_BW = 500;
const long long CHANNELIZER_MAX = 500000;
const long long MIN_VIEW_BW = 30000;
const int DEFAULT_LPS = 30;

enum DragState { DRAG_NONE, DRAG_FREQUENCY, DRAG_BANDWIDTH_LEFT, DRAG_BANDWIDTH_RIGHT, DRAG_RANGE };

struct Carrier {
    long long freqHz;
    double amp;
    double widthHz;
};

struct Demod {
    long long freq = 0;
    int bw = 200000;
    std::string type = "FM";
    bool active = true;
    std::string userLabel;
};

struct AppState {
    long long deviceCenter = 146000000;
    long long sampleRate = 2000000;
    long long deviceMin = 500000;
    long long deviceMax = 1750000000;
    long long offset = 0;
    bool isView = false;
    long long viewCenter = 146000000;
    long long viewBW = 2000000;
    float scaleFactor = 1.0f;
    bool resetScale = false;
    float scaleMove = 0.0f;
    float mouseZoom = 1.0f;
    float zoom = 1.0f;
    bool freqMoving = false;
    float freqMove = 0.0f;
    bool midZooming = false;
    int midZoomStartY = 0;
    std::vector<Demod> demods;
    int selection = -1;
    int hover = -1;
    std::string lastType = "FM";
    int lastBW = 200000;
    int snap = 1;
    DragState dragState = DRAG_NONE;
    DragState nextDragState = DRAG_NONE;
    long long dragOfs = 0;
    int dragBW = 0;
    float hoverAlpha = 1.0f;
    float rightDragDist = 0.0f;
    int lps = DEFAULT_LPS;
    std::vector<Carrier> carriers;
};

long long effCenter(const AppState &app);
long long effBW(const AppState &app);

int ndcToPx(float ndc, int w) {
    return (int)((ndc + 1.0f) * 0.5f * (float)w);
}

int ndcToPy(float ndc, int h) {
    return (int)((1.0f - ndc) * 0.5f * (float)h);
}

Color themeColor(const RGBA4f &c, float a) {
    Color out;
    out.r = (unsigned char)(c.r * 255.0f);
    out.g = (unsigned char)(c.g * 255.0f);
    out.b = (unsigned char)(c.b * 255.0f);
    out.a = (unsigned char)(a * 255.0f);
    return out;
}

double synthMag(long long f, const AppState &app) {
    double mag = 4.0 + (double)(std::rand() % 1000) / 1000.0 * 2.5;
    for (size_t i = 0; i < app.carriers.size(); i++) {
        double d = ((double)f - (double)app.carriers[i].freqHz) / app.carriers[i].widthHz;
        mag += app.carriers[i].amp * std::exp(-d * d);
    }
    return mag;
}

struct WaterfallView {
    Image images[2];
    Texture2D textures[2];
    Color *pixels[2];
    WaterfallRing ring;
    WaterfallPacer pacer;
    std::deque<std::vector<float>> queue;
};

void viewSetup(WaterfallView &view, int lps) {
    view.ring.setup(FFT_SIZE, NUM_LINES);
    for (int i = 0; i < 2; i++) {
        view.images[i] = GenImageColor((int)(FFT_SIZE / 2), NUM_LINES, BLACK);
        view.pixels[i] = (Color *)view.images[i].data;
        view.textures[i] = LoadTextureFromImage(view.images[i]);
    }
    view.ring.setReady(true);
    view.pacer.setLinesPerSecond(lps);
}

void viewShutdown(WaterfallView &view) {
    for (int i = 0; i < 2; i++) {
        UnloadTexture(view.textures[i]);
        UnloadImage(view.images[i]);
    }
}

void viewUpload(WaterfallView &view) {
    WaterfallRing::UploadPlan plan = view.ring.planUpload();
    Gradient &gradient = ThemeMgr::mgr.currentTheme->waterfallGradient;
    std::vector<float> &red = gradient.getRed();
    std::vector<float> &green = gradient.getGreen();
    std::vector<float> &blue = gradient.getBlue();
    for (size_t i = 0; i < plan.ops.size(); i++) {
        const WaterfallRing::UploadOp &op = plan.ops[i];
        const unsigned char *src = view.ring.reversedLine(op.texture) + op.srcOffset;
        for (int r = 0; r < op.height; r++) {
            Color *dst = view.pixels[op.texture] + (size_t)(op.y + r) * (FFT_SIZE / 2);
            for (unsigned int x = 0; x < FFT_SIZE / 2; x++) {
                unsigned char b = src[(size_t)r * (FFT_SIZE / 2) + x];
                dst[x].r = (unsigned char)(red[b] * 255.0f);
                dst[x].g = (unsigned char)(green[b] * 255.0f);
                dst[x].b = (unsigned char)(blue[b] * 255.0f);
                dst[x].a = 255;
            }
        }
        Rectangle rec = {(float)0, (float)op.y, (float)(FFT_SIZE / 2), (float)op.height};
        UpdateTextureRec(view.textures[op.texture], rec,
            view.pixels[op.texture] + (size_t)op.y * (FFT_SIZE / 2));
    }
    view.ring.commitUpload(plan);
}

void viewDraw(WaterfallView &view, int w, int h) {
    int half = (int)(FFT_SIZE / 2);
    int top = view.ring.rowOffset();
    float halfW = (float)w / 2.0f;
    float rowH = (float)h / (float)NUM_LINES;
    for (int i = 0; i < 2; i++) {
        float dx = (i == 0) ? 0.0f : halfW;
        Rectangle srcTop = {(float)0, (float)top, (float)half, (float)(NUM_LINES - top)};
        Rectangle dstTop = {dx, 0.0f, halfW, (float)(NUM_LINES - top) * rowH};
        DrawTexturePro(view.textures[i], srcTop, dstTop, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        if (top > 0) {
            Rectangle srcBottom = {(float)0, (float)0, (float)half, (float)top};
            Rectangle dstBottom = {dx, (float)(NUM_LINES - top) * rowH, halfW, (float)top * rowH};
            DrawTexturePro(view.textures[i], srcBottom, dstBottom, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        }
    }
}

void viewFeed(WaterfallView &view, AppState &app) {
    double yDen = std::log10((POINT_CEIL + 0.25) - (POINT_FLOOR - 0.75));
    long long vc = effCenter(app);
    long long vb = effBW(app);
    std::vector<float> line(FFT_SIZE);
    for (unsigned int i = 0; i < FFT_SIZE; i++) {
        long long f = vc - vb / 2 + (long long)((double)i / (double)FFT_SIZE * (double)vb);
        double norm = std::log10(synthMag(f, app) + 0.25 - (POINT_FLOOR - 0.75)) / yDen;
        line[i] = (float)(norm * app.scaleFactor);
    }
    view.queue.push_back(line);

    WaterfallPacer::Plan plan = view.pacer.plan((double)GetFrameTime(), view.queue.size());
    for (int d = 0; d < plan.dropBefore && !view.queue.empty(); d++) {
        view.queue.pop_front();
    }
    bool updated = false;
    for (int s = 0; s < plan.insert && !view.queue.empty(); s++) {
        view.ring.setPoints(view.queue.front());
        view.queue.pop_front();
        view.ring.step();
        updated = true;
    }
    for (int d = 0; d < plan.dropAfter && !view.queue.empty(); d++) {
        view.queue.pop_front();
    }
    if (updated) {
        viewUpload(view);
    }
}

void drawDemodEdges(const Demod &demod, const RGBA4f &color, const AppState &app, int w, int h) {
    DemodMarkers::MarkerInput in;
    in.freqHz = demod.freq;
    in.bandwidthHz = demod.bw;
    in.sideband = DemodMarkers::sidebandForType(demod.type);
    in.viewCenterHz = effCenter(app);
    in.viewSpanHz = effBW(app);
    in.viewWidthPx = w;
    in.viewHeightPx = h;
    DemodMarkers::MarkerGeometry geo = DemodMarkers::markerGeometry(in);

    Color edge = themeColor(color, 0.6f);
    int xc = ndcToPx(geo.uxPos, w);
    DrawLine(xc, 0, xc, h, edge);
    DrawLine(ndcToPx(geo.uxPos - geo.ofsLeft, w), 0, ndcToPx(geo.uxPos - geo.ofsLeft, w), h, edge);
    DrawLine(ndcToPx(geo.uxPos + geo.ofsRight, w), 0, ndcToPx(geo.uxPos + geo.ofsRight, w), h, edge);

    int x0 = ndcToPx(geo.uxPos - geo.ofsLeft, w);
    int x1 = ndcToPx(geo.uxPos + geo.ofsRight, w);
    DrawRectangle(x0, 0, x1 - x0, h, themeColor(color, 0.2f * app.hoverAlpha));
}

void drawDemodLabel(const Demod &demod, const AppState &app, int w, int h) {
    float uxUnit = (float)(demod.freq - (effCenter(app) - effBW(app) / 2)) / (float)effBW(app);
    std::string label = DemodMarkers::demodTypeLabel(demod.type, false);
    float xOfs = 2.0f / (float)w;
    if (label == "LSB") {
        uxUnit -= xOfs;
    } else if (label == "USB") {
        uxUnit += xOfs;
    }
    float hPos = 20.0f / (float)h;
    int x = ndcToPx(2.0f * (uxUnit - 0.5f), w);
    int y = ndcToPy(-1.0f + hPos * 1.2f, h);
    if (!demod.userLabel.empty()) {
        int ty = ndcToPy(-1.0f + hPos * 1.2f + hPos * 1.2f, h);
        int tw = MeasureText(label.c_str(), 16);
        DrawText(label.c_str(), x - tw / 2 + 1, ty + 1, 16, BLACK);
        DrawText(label.c_str(), x - tw / 2, ty, 16, WHITE);
    }
    const std::string &bottom = demod.userLabel.empty() ? label : demod.userLabel;
    int bw = MeasureText(bottom.c_str(), 16);
    DrawText(bottom.c_str(), x - bw / 2 + 1, y + 1, 16, BLACK);
    DrawText(bottom.c_str(), x - bw / 2, y, 16, WHITE);
}

void drawFreqSelector(float uxPos01, const RGBA4f &color, float w, const AppState &app, int sw, int sh) {
    long long bw = (app.selection >= 0 && (size_t)app.selection < app.demods.size()) ? app.demods[(size_t)app.selection].bw
                                                                                     : app.lastBW;
    float ofs = (w != 0.0f) ? w : ((float)bw / (float)effBW(app));
    float xc = (uxPos01 - 0.5f) * 2.0f;
    Color edge = themeColor(color, 0.6f);
    int x = ndcToPx(xc, sw);
    DrawLine(x, 0, x, sh, edge);
    if (app.lastType != "USB") {
        int xl = ndcToPx(xc - ofs, sw);
        DrawLine(xl, 0, xl, sh, edge);
    }
    if (app.lastType != "LSB") {
        int xr = ndcToPx(xc + ofs, sw);
        DrawLine(xr, 0, xr, sh, edge);
    }
}

void drawRangeSelector(float uxPos1, float uxPos2, const RGBA4f &color, int w, int h, const std::string &lastType) {
    if (uxPos2 < uxPos1) {
        float temp = uxPos2;
        uxPos2 = uxPos1;
        uxPos1 = temp;
    }
    Color edge = themeColor(color, 0.6f);
    Vector2 a0 = {(float)ndcToPx((uxPos1 - 0.5f) * 2.0f, w), 0.0f};
    Vector2 a1 = {(float)ndcToPx((uxPos1 - 0.5f) * 2.0f, w), (float)h};
    DrawLineEx(a0, a1, (lastType == "USB") ? 2.0f : 1.0f, edge);
    Vector2 b0 = {(float)ndcToPx((uxPos2 - 0.5f) * 2.0f, w), 0.0f};
    Vector2 b1 = {(float)ndcToPx((uxPos2 - 0.5f) * 2.0f, w), (float)h};
    DrawLineEx(b0, b1, (lastType == "LSB") ? 2.0f : 1.0f, edge);
}

long long effCenter(const AppState &app) {
    return app.isView ? app.viewCenter : app.deviceCenter;
}

long long effBW(const AppState &app) {
    return app.isView ? app.viewBW : app.sampleRate;
}

void deviceSetCenter(AppState &app, long long freq) {
    app.deviceCenter = clampTuneFrequency(freq, app.offset, deviceTuneFloor(app.deviceMin, app.sampleRate), app.deviceMax);
    if (std::llabs(app.deviceCenter - app.viewCenter) > app.sampleRate / 2) {
        app.viewCenter = app.deviceCenter;
    }
}

void updateCenterFrequency(AppState &app, long long freq) {
    if (app.isView) {
        app.viewCenter = freq;
        long long minFreq = app.deviceCenter - app.sampleRate / 2;
        long long maxFreq = app.deviceCenter + app.sampleRate / 2;
        if (freq - app.viewBW / 2 < minFreq) {
            deviceSetCenter(app, app.deviceCenter - (minFreq - (freq - app.viewBW / 2)));
        }
        if (freq + app.viewBW / 2 > maxFreq) {
            deviceSetCenter(app, app.deviceCenter + ((freq + app.viewBW / 2) - maxFreq));
        }
        long long pinned = pinViewCenter(freq, app.viewBW, app.deviceCenter, app.sampleRate);
        if (pinned != freq) {
            app.viewCenter = pinned;
        }
    } else {
        deviceSetCenter(app, freq);
    }
}

void updateHoverState(AppState &app, float mx, bool altDown, bool shiftDown) {
    long long freqPos = WaterfallDrag::freqAtX(mx, effCenter(app), effBW(app));
    app.hover = -1;
    if (altDown) {
        app.nextDragState = DRAG_RANGE;
        return;
    }
    std::vector<int> nearby;
    for (size_t i = 0; i < app.demods.size(); i++) {
        const Demod &d = app.demods[i];
        long long halfBw = d.bw / 2;
        long long lo = d.freq - (d.type != "USB" ? halfBw : 0) - 7500;
        long long hi = d.freq + (d.type != "LSB" ? halfBw : 0) + 7500;
        if (freqPos >= lo && freqPos <= hi) {
            nearby.push_back((int)i);
        }
    }
    if (!nearby.empty() && !shiftDown) {
        std::vector<WaterfallHover::HoverCandidate> candidates;
        for (size_t i = 0; i < nearby.size(); i++) {
            const Demod &d = app.demods[(size_t)nearby[i]];
            WaterfallHover::HoverCandidate cand;
            cand.freqHz = d.freq;
            cand.bandwidthHz = d.bw;
            cand.sideband = DemodMarkers::sidebandForType(d.type);
            candidates.push_back(cand);
        }
        WaterfallHover::HoverPick pick = WaterfallHover::pickHover(candidates, freqPos, effBW(app), app.sampleRate);
        if (pick.zone == WaterfallHover::DragZone::None) {
            app.nextDragState = DRAG_NONE;
            return;
        }
        app.hover = nearby[(size_t)pick.index];
        if (pick.zone == WaterfallHover::DragZone::Frequency) {
            app.nextDragState = DRAG_FREQUENCY;
        } else if (pick.zone == WaterfallHover::DragZone::BandwidthLeft) {
            app.nextDragState = DRAG_BANDWIDTH_LEFT;
        } else if (pick.zone == WaterfallHover::DragZone::BandwidthRight) {
            app.nextDragState = DRAG_BANDWIDTH_RIGHT;
        }
    } else {
        app.nextDragState = DRAG_NONE;
    }
}

void pressLeft(AppState &app, float mx, bool altDown, bool shiftDown) {
    updateHoverState(app, mx, altDown, shiftDown);
    app.dragState = app.nextDragState;
    if (app.dragState != DRAG_NONE && app.dragState != DRAG_RANGE) {
        if (app.hover >= 0) {
            app.selection = app.hover;
        }
        if (app.selection >= 0 && (size_t)app.selection < app.demods.size()) {
            app.dragOfs = WaterfallDrag::dragOffset(mx, effBW(app), effCenter(app), app.demods[(size_t)app.selection].freq);
            app.dragBW = app.demods[(size_t)app.selection].bw;
        }
    }
}

void dragLeft(AppState &app, float mx, float dx) {
    if (app.selection < 0 || (size_t)app.selection >= app.demods.size()) {
        return;
    }
    Demod &demod = app.demods[(size_t)app.selection];
    if (app.dragState == DRAG_BANDWIDTH_LEFT || app.dragState == DRAG_BANDWIDTH_RIGHT) {
        int currentBW = WaterfallDrag::bandwidthDragStep(dx, effBW(app), app.dragState == DRAG_BANDWIDTH_LEFT,
            app.dragBW, (int)MIN_BW, (int)CHANNELIZER_MAX);
        demod.bw = currentBW;
        app.dragBW = currentBW;
    }
    if (app.dragState == DRAG_FREQUENCY) {
        long long bwTarget = WaterfallDrag::freqAtX(mx, effCenter(app), effBW(app)) - app.dragOfs;
        long long bwDiff = WaterfallDrag::snapDelta(bwTarget - demod.freq, app.snap);
        if (bwDiff != 0) {
            demod.freq = pinDemodCenter(demod.freq + bwDiff, effCenter(app), effBW(app));
        }
    }
}

void releaseLeft(AppState &app, float mx, float originX, float originDeltaX, float originDeltaY, bool shiftDown) {
    bool haveSel = app.selection >= 0 && (size_t)app.selection < app.demods.size();
    bool isNew = shiftDown || !haveSel;
    if (haveSel && !app.demods[(size_t)app.selection].active) {
        isNew = true;
    }
    if (originDeltaX == 0.0f && originDeltaY == 0.0f) {
        bool haveDemod = !isNew && haveSel;
        long long freq = WaterfallDrag::clickPlaceFreq(mx, effCenter(app), effBW(app), haveDemod,
            haveDemod ? app.demods[(size_t)app.selection].freq : 0, app.snap);
        if (app.dragState == DRAG_NONE) {
            int idx;
            if (!isNew && !app.demods.empty()) {
                idx = app.selection;
            } else {
                isNew = true;
                Demod nd;
                nd.freq = freq;
                nd.type = app.lastType;
                nd.bw = app.lastBW;
                nd.active = true;
                app.demods.push_back(nd);
                idx = (int)app.demods.size() - 1;
            }
            app.demods[(size_t)idx].freq = freq;
            app.selection = idx;
            app.nextDragState = DRAG_FREQUENCY;
        } else {
            if (haveDemod) {
                app.nextDragState = DRAG_FREQUENCY;
            } else {
                app.nextDragState = DRAG_NONE;
            }
        }
    } else if (app.dragState == DRAG_RANGE) {
        WaterfallDrag::RangeDrag range = WaterfallDrag::rangeDrag(originX, mx, originDeltaX,
            DemodMarkers::sidebandForType(app.lastType), effCenter(app), effBW(app), app.snap, (unsigned int)MIN_BW);
        if (range.bandwidthHz == 0) {
            app.dragState = DRAG_NONE;
            return;
        }
        long long freq = pinDemodCenter(range.freqHz, effCenter(app), effBW(app));
        int idx;
        if (!isNew && !app.demods.empty()) {
            idx = app.selection;
        } else {
            Demod nd;
            nd.type = app.lastType;
            nd.active = true;
            app.demods.push_back(nd);
            idx = (int)app.demods.size() - 1;
        }
        app.demods[(size_t)idx].freq = freq;
        app.demods[(size_t)idx].bw = (int)range.bandwidthHz;
        app.selection = idx;
    }
    app.dragState = DRAG_NONE;
}

void applyZoom(AppState &app, float currentZoom, float mpos, bool mouseInView) {
    long long bw = effBW(app);
    if (currentZoom < 1.0f) {
        bw = (long long)ceil((long double)bw * (long double)currentZoom);
        if (bw < MIN_VIEW_BW) {
            bw = MIN_VIEW_BW;
        }
        if (mouseInView) {
            long long mfreqA = WaterfallDrag::freqAtX(mpos, app.viewCenter, app.viewBW);
            long long mfreqB = WaterfallDrag::freqAtX(mpos, app.viewCenter, bw);
            app.viewCenter += mfreqA - mfreqB;
        }
        app.isView = true;
        app.viewBW = bw;
    } else {
        if (app.isView) {
            bw = (long long)ceil((long double)bw * (long double)currentZoom);
            if (bw >= app.sampleRate) {
                app.isView = false;
                bw = app.sampleRate;
                app.viewCenter = app.deviceCenter;
                app.viewBW = bw;
            } else {
                if (mouseInView) {
                    long long mfreqA = WaterfallDrag::freqAtX(mpos, app.viewCenter, app.viewBW);
                    long long mfreqB = WaterfallDrag::freqAtX(mpos, app.viewCenter, bw);
                    app.viewCenter += mfreqA - mfreqB;
                }
                app.viewBW = bw;
            }
        }
    }
    long long freq = app.deviceCenter;
    if (app.viewCenter < freq && (app.viewCenter - app.viewBW / 2) < (freq - app.sampleRate / 2)) {
        app.viewCenter = (freq - app.sampleRate / 2) + app.viewBW / 2;
    }
    if (app.viewCenter > freq && (app.viewCenter + app.viewBW / 2) > (freq + app.sampleRate / 2)) {
        app.viewCenter = (freq + app.sampleRate / 2) - app.viewBW / 2;
    }
}

const char *dragName(DragState state) {
    switch (state) {
    case DRAG_FREQUENCY:
        return "FREQ";
    case DRAG_BANDWIDTH_LEFT:
        return "BWL";
    case DRAG_BANDWIDTH_RIGHT:
        return "BWR";
    case DRAG_RANGE:
        return "RANGE";
    default:
        return "NONE";
    }
}

} // namespace

int main() {
    std::srand(42);
    AppState app;
    Demod fm;
    fm.freq = 146520000;
    fm.bw = 200000;
    fm.type = "FM";
    fm.active = true;
    app.demods.push_back(fm);
    Demod am;
    am.freq = 145700000;
    am.bw = 10000;
    am.type = "AM";
    am.active = true;
    am.userLabel = "TEST";
    app.demods.push_back(am);
    Demod usb;
    usb.freq = 146900000;
    usb.bw = 2700;
    usb.type = "USB";
    usb.active = true;
    app.demods.push_back(usb);
    app.selection = 0;
    app.carriers.push_back(Carrier{146520000, 85.0, 25000.0});
    app.carriers.push_back(Carrier{145700000, 45.0, 12000.0});
    app.carriers.push_back(Carrier{146900000, 20.0, 60000.0});
    app.carriers.push_back(Carrier{145200000, 10.0, 8000.0});

    InitWindow(900, 500, "CubicSDR waterfall spike (raylib, no wx)");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    WaterfallView view;
    viewSetup(view, app.lps);

    bool leftDown = false, rightDown = false;
    bool wasInView = true;
    float originX = 0.0f, originY = 0.0f;
    float lastMx = 0.0f, lastMy = 0.0f;
    float wheelFrac = 0.0f;

    while (!WindowShouldClose()) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        Vector2 mouse = GetMousePosition();
        float mx = mouse.x / (float)w;
        float my = 1.0f - mouse.y / (float)h;
        bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        bool altDown = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
        bool bounds = mouse.x >= 0 && mouse.x < (float)w && mouse.y >= 0 && mouse.y < (float)h;
        bool inView = bounds || leftDown || rightDown;
        if (leftDown) {
            my = originY;
        }

        if (wasInView && !inView) {
            app.hover = -1;
            app.hoverAlpha = 0.0f;
            app.mouseZoom = 1.0f;
            if (app.midZooming) {
                app.midZooming = false;
                app.zoom = 1.0f;
            }
        }
        wasInView = inView;
        if (app.midZooming && !bounds) {
            app.midZooming = false;
            app.zoom = 1.0f;
        }

        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_KP_8)) {
            if (!shiftDown) {
                app.mouseZoom = 1.0f;
                app.zoom = 0.95f;
            } else {
                app.scaleMove = 1.0f;
            }
        }
        if (IsKeyReleased(KEY_UP) || IsKeyReleased(KEY_KP_8)) {
            app.scaleMove = 0.0f;
            app.zoom = 1.0f;
            if (app.mouseZoom != 1.0f) {
                app.mouseZoom = 0.95f;
            }
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_KP_2)) {
            if (!shiftDown) {
                app.mouseZoom = 1.0f;
                app.zoom = 1.05f;
            } else {
                app.scaleMove = -1.0f;
            }
        }
        if (IsKeyReleased(KEY_DOWN) || IsKeyReleased(KEY_KP_2)) {
            app.scaleMove = 0.0f;
            app.zoom = 1.0f;
            if (app.mouseZoom != 1.0f) {
                app.mouseZoom = 1.05f;
            }
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_KP_6)) {
            if (app.isView) {
                app.freqMove = shiftDown ? 5.0f : 1.0f;
                app.freqMoving = true;
            } else {
                deviceSetCenter(app, app.deviceCenter + (shiftDown ? effBW(app) * 10 : effBW(app) / 2));
            }
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_KP_4)) {
            if (app.isView) {
                app.freqMove = shiftDown ? -5.0f : -1.0f;
                app.freqMoving = true;
            } else {
                deviceSetCenter(app, app.deviceCenter - (shiftDown ? effBW(app) * 10 : effBW(app) / 2));
            }
        }
        if (IsKeyReleased(KEY_LEFT) || IsKeyReleased(KEY_KP_4) || IsKeyReleased(KEY_RIGHT) || IsKeyReleased(KEY_KP_6)) {
            app.freqMoving = false;
        }
        if (IsKeyPressed(KEY_C)) {
            if (app.hover >= 0 && (size_t)app.hover < app.demods.size()) {
                deviceSetCenter(app, app.demods[(size_t)app.hover].freq);
            } else if (inView) {
                long long nfreq = WaterfallDrag::freqAtX(mx, effCenter(app), effBW(app));
                if (app.snap > 1) {
                    nfreq = (long long)(roundf((float)nfreq / (float)app.snap) * (float)app.snap);
                }
                deviceSetCenter(app, nfreq);
            }
        }
        if (IsKeyPressed(KEY_S)) {
            if (app.snap == 1) {
                app.snap = 1000;
            } else if (app.snap == 1000) {
                app.snap = 10000;
            } else if (app.snap == 10000) {
                app.snap = 100000;
            } else {
                app.snap = 1;
            }
        }
        if (IsKeyPressed(KEY_DELETE)) {
            if (app.selection >= 0 && (size_t)app.selection < app.demods.size()) {
                app.demods.erase(app.demods.begin() + app.selection);
                app.selection = -1;
            }
        }
        if (IsKeyPressed(KEY_EQUAL)) {
            app.lps += 5;
            if (app.lps > 120) {
                app.lps = 120;
            }
            view.pacer.setLinesPerSecond(app.lps);
            view.queue.clear();
        }
        if (IsKeyPressed(KEY_MINUS)) {
            app.lps -= 5;
            if (app.lps < 5) {
                app.lps = 5;
            }
            view.pacer.setLinesPerSecond(app.lps);
            view.queue.clear();
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            leftDown = true;
            originX = mx;
            originY = my;
            lastMx = mx;
            pressLeft(app, mx, altDown, shiftDown);
        }
        if (leftDown) {
            float dx = mx - lastMx;
            lastMx = mx;
            dragLeft(app, mx, dx);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            leftDown = false;
            releaseLeft(app, mx, originX, mx - originX, my - originY, shiftDown);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            rightDown = true;
            lastMy = my;
            app.rightDragDist = 0.0f;
        }
        if (rightDown) {
            float dy = my - lastMy;
            lastMy = my;
            app.rightDragDist += (float)fabs(dy);
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
            if (app.rightDragDist < 0.01f) {
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

        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
            app.midZooming = true;
            app.midZoomStartY = (int)mouse.y;
        }
        if (app.midZooming) {
            app.zoom = WaterfallDrag::glideZoom(app.midZoomStartY, (int)mouse.y, h);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
            app.midZooming = false;
            app.zoom = 1.0f;
        }

        wheelFrac += GetMouseWheelMove() * 120.0f;
        int rotation = (int)wheelFrac;
        wheelFrac -= (float)rotation;
        if (rotation != 0) {
            app.mouseZoom = WaterfallDrag::wheelZoom(rotation, 120);
        }

        if (!leftDown && !rightDown && !app.midZooming) {
            updateHoverState(app, mx, altDown, shiftDown);
        }

        DragState cursorState = leftDown ? app.dragState : app.nextDragState;
        if (cursorState == DRAG_FREQUENCY) {
            SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
        } else if (cursorState == DRAG_BANDWIDTH_LEFT || cursorState == DRAG_BANDWIDTH_RIGHT) {
            SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
        } else {
            SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
        }

        viewFeed(view, app);

        float currentZoom = app.zoom;
        if (app.mouseZoom != 1.0f) {
            currentZoom = app.mouseZoom;
            app.mouseZoom = app.mouseZoom + (1.0f - app.mouseZoom) * 0.2f;
            if (fabs(app.mouseZoom - 1.0f) < 0.01f) {
                app.mouseZoom = 1.0f;
            }
        }

        if (app.scaleMove != 0.0f) {
            app.scaleFactor += app.scaleMove * 0.02f;
            if (app.scaleFactor < 0.25f) {
                app.scaleFactor = 0.25f;
            }
            if (app.scaleFactor > 10.0f) {
                app.scaleFactor = 10.0f;
            }
        }

        if (app.freqMove != 0.0f) {
            long long newFreq = effCenter(app) + (long long)((long double)effBW(app) * app.freqMove) * 0.01;
            updateCenterFrequency(app, newFreq);
            if (!app.freqMoving) {
                app.freqMove -= app.freqMove * 0.2f;
                if (fabs(app.freqMove) < 0.01f) {
                    app.freqMove = 0.0f;
                }
            }
        }

        if (currentZoom != 1.0f) {
            applyZoom(app, currentZoom, mx, inView);
        }

        bool haveSel = app.selection >= 0 && (size_t)app.selection < app.demods.size();
        bool haveHover = app.hover >= 0 && (size_t)app.hover < app.demods.size();
        bool isNewPaint = shiftDown || (haveSel && !app.demods[(size_t)app.selection].active);
        bool dimSelection = haveSel && ((haveHover && app.hover != app.selection) || (!haveHover && isNewPaint));

        if (inView || haveHover) {
            app.hoverAlpha += (1.0f - app.hoverAlpha) * 0.1f;
            if (app.hoverAlpha > 1.5f) {
                app.hoverAlpha = 1.5f;
            }
        } else {
            app.hoverAlpha += (0.0f - app.hoverAlpha) * 0.05f;
            if (app.hoverAlpha < 1.0e-5f) {
                app.hoverAlpha = 0.0f;
            }
        }

        ColorTheme *theme = ThemeMgr::mgr.currentTheme;
        struct OverlayDraw {
            int demod;
            RGBA4f color;
        };
        std::vector<OverlayDraw> draws;
        bool showFreqSelector = false;
        float selectorPos = 0.0f;
        RGBA4f selectorColor = theme->waterfallHover;
        float selectorWidth = 0.0f;
        bool showRangeSelector = false;
        float rangePos1 = 0.0f, rangePos2 = 0.0f;

        if (inView || haveHover) {
            if (app.nextDragState == DRAG_RANGE) {
                float width = 1.0f / (float)w;
                float rangeWidth = mx - originX;
                float centerPos;
                if (leftDown) {
                    if (rangeWidth != 0.0f) {
                        width = rangeWidth;
                    }
                    centerPos = originX + width / 2.0f;
                } else {
                    centerPos = mx;
                }
                if (haveSel) {
                    OverlayDraw draw;
                    draw.demod = app.selection;
                    draw.color = isNewPaint ? theme->waterfallHighlight : theme->waterfallDestroy;
                    draws.push_back(draw);
                }
                if ((app.lastType == "LSB" || app.lastType == "USB") && leftDown) {
                    centerPos = mx;
                    showRangeSelector = true;
                    rangePos1 = centerPos;
                    rangePos2 = centerPos - width;
                    selectorColor = isNewPaint ? theme->waterfallNew : theme->waterfallHover;
                } else {
                    showFreqSelector = true;
                    selectorPos = centerPos;
                    selectorWidth = width;
                    selectorColor = isNewPaint ? theme->waterfallNew : theme->waterfallHover;
                }
            } else {
                if (haveSel && !dimSelection) {
                    OverlayDraw draw;
                    draw.demod = app.selection;
                    bool highlight = (isNewPaint && !haveHover) || haveHover;
                    draw.color = highlight ? theme->waterfallHighlight : theme->waterfallDestroy;
                    draws.push_back(draw);
                }
                if (!haveHover) {
                    showFreqSelector = true;
                    selectorPos = mx;
                    selectorWidth = 0.0f;
                    selectorColor = ((isNewPaint && haveSel) || !haveSel) ? theme->waterfallNew : theme->waterfallHover;
                } else if (app.hover != app.selection) {
                    OverlayDraw draw;
                    draw.demod = app.hover;
                    draw.color = theme->waterfallHover;
                    draws.push_back(draw);
                }
            }
        } else {
            if (haveSel) {
                OverlayDraw draw;
                draw.demod = app.selection;
                draw.color = theme->waterfallHighlight;
                draws.push_back(draw);
            }
        }

        for (size_t i = 0; i < app.demods.size(); i++) {
            if (!app.demods[i].active && (int)i != app.selection) {
                continue;
            }
            if ((int)i == app.hover || ((int)i == app.selection && !dimSelection)) {
                continue;
            }
            OverlayDraw draw;
            draw.demod = (int)i;
            draw.color = theme->waterfallHighlight;
            draws.push_back(draw);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        viewDraw(view, w, h);

        BeginBlendMode(BLEND_ADDITIVE);
        for (size_t i = 0; i < draws.size(); i++) {
            drawDemodEdges(app.demods[(size_t)draws[i].demod], draws[i].color, app, w, h);
        }
        if (showFreqSelector) {
            drawFreqSelector(selectorPos, selectorColor, selectorWidth, app, w, h);
        }
        if (showRangeSelector) {
            drawRangeSelector(rangePos1, rangePos2, selectorColor, w, h, app.lastType);
        }
        EndBlendMode();

        for (size_t i = 0; i < draws.size(); i++) {
            drawDemodLabel(app.demods[(size_t)draws[i].demod], app, w, h);
        }

        char status[256];
        snprintf(status, sizeof(status), "view=%lld/%lld dev=%lld/%lld lps=%d sf=%.2f", effCenter(app), effBW(app),
            app.deviceCenter, app.sampleRate, app.lps, app.scaleFactor);
        char state[256];
        snprintf(state, sizeof(state), "sel=%d hov=%d drag=%s/%s snap=%d ndemod=%d", app.selection, app.hover,
            dragName(app.dragState), dragName(app.nextDragState), app.snap, (int)app.demods.size());
        const char *keys = "drag move/bw ALT range SH click new wheel/mid/arrows zoom R gain C center S snap DEL del -/= lps";
        DrawRectangle(4, h - 54, MeasureText(keys, 10) + 8, 50, Fade(BLACK, 0.75f));
        DrawText(status, 8, h - 52, 10, GRAY);
        DrawText(state, 8, h - 38, 10, GRAY);
        DrawText(keys, 8, h - 24, 10, GRAY);

        EndDrawing();
    }

    viewShutdown(view);
    CloseWindow();
    return 0;
}
