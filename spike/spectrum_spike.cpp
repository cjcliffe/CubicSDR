// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "imgui_bgfx.h"
#include "imgui_draw_util.h"

#include <imgui.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#include <DemodMarkers.h>
#include <SpectrumTicks.h>
#include <WaterfallDrag.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct SpikeTheme {
    ThemeColor fftBackground;
    ThemeColor fftFill;
    ThemeColor fftLine;
    ThemeColor fftHighlight;
    ThemeColor freqLine;
    ThemeColor text;
    ThemeColor waterfallHover;
    ThemeColor waterfallNew;
    const char *name;
};

const SpikeTheme kThemes[] = {
    {{0.1f, 0.1f, 0.1f, 1.0f}, {0.15f, 0.35f, 0.85f, 0.3f}, {0.9f, 0.9f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f},
        "default"},
    {{0.05f, 0.05f, 0.15f, 1.0f}, {0.25f, 0.5f, 1.0f, 0.25f}, {0.9f, 0.9f, 1.0f, 1.0f},
        {0.9f, 0.9f, 1.0f, 1.0f}, {0.85f, 0.85f, 1.0f, 1.0f}, {0.9f, 0.9f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f}, "sharp"},
    {{0.0f, 0.0f, 50.0f / 255.0f, 1.0f}, {0.85f, 0.3f, 0.1f, 0.25f}, {1.0f, 0.6f, 0.3f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f}, "rad"},
};

struct FakeDemod {
    long long freq;
    int bw;
    const char *type;
    const char *label;
    bool selected = false;
    bool muted = false;
    bool solo = false;
    bool recording = false;
    bool deltaLock = false;
};

struct AppState {
    long long centerFreq = 100000000;
    long long spanHz = 2000000;
    float scaleFactor = 1.0f;
    bool resetScale = false;
    bool showDb = true;
    bool showPeak = true;
    int theme = 0;
    bool ghostNew = false;
    int snap = 1;
    bool leftDown = false;
    bool rightDown = false;
    float lastPanX01 = 0.0f;
    float lastGainY01 = 0.0f;
    float rightOriginY01 = 0.0f;
    float rightDragDist = 0.0f;
    float floorValue = 8.0f;
    float ceilValue = 400.0f;
    int fftSize = 2048;
    std::vector<float> points;
    std::vector<float> peak;
    std::vector<FakeDemod> demods;
};

struct FrameEdges {
    bool leftPressed = false;
    bool leftReleased = false;
    bool rightPressed = false;
    bool rightReleased = false;
    int wheelRotV = 0;
    bool mouseLeft = false;
    bool bPressed = false;
    bool pPressed = false;
    bool tPressed = false;
    bool nPressed = false;
    bool sPressed = false;
};

TextAlign toTextAlign(DemodMarkers::LabelAlign align) {
    if (align == DemodMarkers::LabelAlign::Right) {
        return TextAlign::Right;
    }
    if (align == DemodMarkers::LabelAlign::Center) {
        return TextAlign::Center;
    }
    return TextAlign::Left;
}

void initDemods(AppState &s) {
    s.demods = {
        {100100000, 200000, "FM", "100.100", true, false, false, false, false},
        {99950000, 3000, "USB", "99.950", false, false, false, false, false},
        {100300000, 3000, "LSB", "100.300", false, false, false, false, false},
        {99800000, 150000, "FM", "99.800", false, true, false, false, false},
        {100450000, 12000, "AM", "100.450", false, false, false, true, false},
        {99700000, 25000, "FM", "99.700", false, false, true, false, false},
        {100020000, 5000, "AM", "100.020", false, false, false, false, true},
    };
}

void synthSpectrum(AppState &s, float t) {
    s.ceilValue = 260.0f + 240.0f * sinf(t * 0.3f);
    s.floorValue = 8.0f + 6.0f * sinf(t * 0.23f + 1.0f);
    const int n = 1024;
    if (s.points.size() != (size_t)n * 2) {
        s.points.resize(n * 2);
        s.peak.assign(n * 2, 0.0f);
    }
    for (int i = 0; i < n; i++) {
        float x = (float)i / (float)n;
        float v = 0.06f + 0.03f * sinf(x * 91.0f + t * 3.1f) * sinf(x * 47.0f - t * 1.7f);
        float dx0 = (x - (0.30f + 0.02f * sinf(t * 0.4f))) / 0.012f;
        float dx1 = (x - (0.55f + 0.015f * sinf(t * 0.23f))) / 0.006f;
        float dx2 = (x - 0.72f) / 0.03f;
        v += 0.55f * expf(-dx0 * dx0);
        v += 0.75f * expf(-dx1 * dx1);
        v += 0.35f * expf(-dx2 * dx2) * (0.7f + 0.3f * sinf(t * 2.0f));
        v *= s.scaleFactor;
        s.points[i * 2] = x;
        s.points[i * 2 + 1] = v;
        float pk = s.peak[i * 2 + 1];
        pk = v > pk ? v : pk - 0.001f;
        if (pk < 0.0f) {
            pk = 0.0f;
        }
        s.peak[i * 2] = x;
        s.peak[i * 2 + 1] = pk;
    }
}

void drawSpectrum(AppState &s, const FrameEdges &edges, float mx, float my, int ww, int wh, long long timeMs) {
    const SpikeTheme &theme = kThemes[s.theme];
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)ww, (float)wh), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Spectrum", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    SpecFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, wsize.x, wsize.y};
    bool hovered = !edges.mouseLeft && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    float mx01 = (mx - origin.x) / wsize.x;
    float my01 = 1.0f - (my - origin.y) / wsize.y;

    if (edges.mouseLeft) {
        s.leftDown = false;
        s.rightDown = false;
    }
    if (edges.bPressed) {
        s.showDb = !s.showDb;
    }
    if (edges.pPressed) {
        s.showPeak = !s.showPeak;
    }
    if (edges.tPressed) {
        s.theme = (s.theme + 1) % 3;
    }
    if (edges.nPressed) {
        s.ghostNew = !s.ghostNew;
    }
    if (edges.sPressed) {
        s.snap = s.snap > 1 ? 1 : 12500;
    }
    if (edges.leftPressed && hovered) {
        s.leftDown = true;
        s.lastPanX01 = mx01;
    }
    if (s.leftDown && hovered) {
        float dx = mx01 - s.lastPanX01;
        s.centerFreq -= (long long)((double)dx * (double)s.spanHz);
        s.lastPanX01 = mx01;
    }
    if (edges.leftReleased) {
        s.leftDown = false;
    }
    if (edges.wheelRotV != 0 && hovered) {
        s.spanHz = (long long)((double)s.spanHz * (double)WaterfallDrag::wheelZoom(edges.wheelRotV, 120));
        if (s.spanHz < 20000) {
            s.spanHz = 20000;
        }
        if (s.spanHz > 12000000) {
            s.spanHz = 12000000;
        }
    }
    if (edges.rightPressed && hovered) {
        s.rightDown = true;
        s.lastGainY01 = my01;
        s.rightOriginY01 = my01;
        s.rightDragDist = 0.0f;
    }
    if (s.rightDown && hovered) {
        float dy = my01 - s.lastGainY01;
        s.scaleFactor += dy * 2.0f;
        if (s.scaleFactor < 0.25f) {
            s.scaleFactor = 0.25f;
        }
        if (s.scaleFactor > 10.0f) {
            s.scaleFactor = 10.0f;
        }
        s.rightDragDist += fabsf(dy);
        s.lastGainY01 = my01;
        s.resetScale = false;
    }
    if (edges.rightReleased) {
        if (s.rightDown && (my01 - s.rightOriginY01) == 0.0f) {
            s.resetScale = true;
        }
        s.rightDown = false;
    }
    if (s.resetScale) {
        s.scaleFactor += (1.0f - s.scaleFactor) * 0.05f;
        if (fabsf(s.scaleFactor - 1.0f) < 0.01f) {
            s.scaleFactor = 1.0f;
            s.resetScale = false;
        }
    }

    imguiSetClearColor(packClear(theme.fftBackground));
    ImU32 bgTop = packColor(theme.fftBackground.r, theme.fftBackground.g, theme.fftBackground.b);
    ImU32 bgBottom = packColor(theme.fftBackground.r * 2.0f, theme.fftBackground.g * 2.0f,
        theme.fftBackground.b * 2.0f);
    f.dl->AddRectFilledMultiColor(ImVec2(f.nx(-1.0f), f.ny(1.0f)), ImVec2(f.nx(1.0f), f.ny(-1.0f)), bgTop, bgTop,
        bgBottom, bgBottom);

    if (!s.points.empty()) {
        SpectrumTicks::GridGeometry grid = SpectrumTicks::gridGeometry(s.floorValue, s.ceilValue);
        for (auto &band : grid.bands) {
            for (size_t i = 0; i + 3 < band.verts.size(); i += 4) {
                float ndcY = -0.75f + band.verts[i + 1] * 1.5f;
                float t = (1.0f - ndcY) * 0.5f;
                float lift = 0.12f * (float)band.alpha;
                ImU32 gridCol = packColor(theme.fftBackground.r * (1.0f + t) + lift,
                    theme.fftBackground.g * (1.0f + t) + lift,
                    theme.fftBackground.b * (1.0f + t) + lift);
                f.dl->AddLine(ImVec2(f.px(band.verts[i]), f.py(band.verts[i + 1])),
                    ImVec2(f.px(band.verts[i + 2]), f.py(band.verts[i + 3])), gridCol, 1.0f);
            }
        }

        ImU32 fillCol =
            packColor(theme.fftFill.r, theme.fftFill.g, theme.fftFill.b, theme.fftFill.a);
        size_t nPts = s.points.size() / 2;
        float bottomY = f.py(0.0f);
        size_t seg = 0;
        for (float cx = 0.0f; cx < wsize.x; cx += 1.0f) {
            float x01 = (cx + 0.5f) / wsize.x;
            while (seg + 1 < nPts && s.points[(seg + 1) * 2] < x01) {
                seg++;
            }
            size_t s0 = seg * 2;
            size_t s1 = s0 + 2 < s.points.size() ? s0 + 2 : s0;
            float x0 = s.points[s0];
            float x1 = s.points[s1];
            float y = s.points[s0 + 1];
            if (x1 > x0) {
                float u = (x01 - x0) / (x1 - x0);
                if (u < 0.0f) {
                    u = 0.0f;
                }
                if (u > 1.0f) {
                    u = 1.0f;
                }
                y = s.points[s0 + 1] + (s.points[s1 + 1] - s.points[s0 + 1]) * u;
            }
            float topY = f.py(y);
            if (topY > bottomY) {
                topY = bottomY;
            }
            f.dl->AddRectFilled(ImVec2(f.ox + cx, topY), ImVec2(f.ox + cx + 1.0f, bottomY), fillCol);
        }
        ImU32 skirtTop = packColor(theme.fftFill.r, theme.fftFill.g, theme.fftFill.b, theme.fftFill.a);
        ImU32 skirtBottom = packColor(theme.fftBackground.r, theme.fftBackground.g, theme.fftBackground.b,
            theme.fftFill.a);
        f.dl->AddRectFilledMultiColor(ImVec2(f.nx(-1.0f), f.ny(-0.75f)), ImVec2(f.nx(1.0f), f.ny(-1.0f)), skirtTop,
            skirtTop, skirtBottom, skirtBottom);

        ImU32 lineCol = packColor(theme.fftLine.r, theme.fftLine.g, theme.fftLine.b);
        for (size_t i = 0; i + 1 < nPts; i++) {
            f.dl->AddLine(ImVec2(f.px(s.points[i * 2]), f.py(s.points[i * 2 + 1])),
                ImVec2(f.px(s.points[i * 2 + 2]), f.py(s.points[i * 2 + 3])), lineCol, 1.0f);
        }
        if (s.showPeak && !s.peak.empty()) {
            size_t nHold = s.peak.size() / 2;
            for (size_t i = 0; i + 1 < nHold; i++) {
                f.dl->AddLine(ImVec2(f.px(s.peak[i * 2]), f.py(s.peak[i * 2 + 1])),
                    ImVec2(f.px(s.peak[i * 2 + 2]), f.py(s.peak[i * 2 + 3])), IM_COL32(0, 255, 0, 128), 1.0f);
            }
        }
    }

    SpectrumTicks::AxisGeometry axis =
        SpectrumTicks::axisGeometry(s.centerFreq, s.spanHz, (double)wsize.x, (double)wsize.y, 1.0);
    ImU32 textCol = packColor(theme.text.r, theme.text.g, theme.text.b);
    ImU32 majorCol = packColor(theme.freqLine.r, theme.freqLine.g, theme.freqLine.b);
    ImU32 minorCol =
        packColor(theme.freqLine.r * 0.65f, theme.freqLine.g * 0.65f, theme.freqLine.b * 0.65f);
    for (auto &tick : axis.ticks) {
        drawCenterText(f.dl, tick.label.c_str(), f.nx((float)tick.x), f.ny((float)axis.hPos),
            (float)axis.fontSize, textCol);
    }
    for (auto &tick : axis.ticks) {
        float tx = f.nx((float)tick.x);
        f.dl->AddLine(ImVec2(tx, f.ny((float)axis.tickBase)), ImVec2(tx, f.ny(1.0f)),
            tick.major ? majorCol : minorCol, tick.major ? 4.0f : 1.0f);
    }

    int viewW = (int)wsize.x;
    int viewH = (int)wsize.y;
    for (auto &demod : s.demods) {
        const ThemeColor &color = demod.selected ? theme.waterfallHover : theme.fftHighlight;
        DemodMarkers::MarkerInput in;
        in.freqHz = demod.freq;
        in.bandwidthHz = demod.bw;
        in.sideband = DemodMarkers::sidebandForType(demod.type);
        in.viewCenterHz = s.centerFreq;
        in.viewSpanHz = s.spanHz;
        in.viewWidthPx = viewW;
        in.viewHeightPx = viewH;
        in.deltaLocked = demod.deltaLock;
        in.muted = demod.muted;
        in.solo = demod.solo;
        in.recording = demod.recording;
        in.timeMs = timeMs;
        DemodMarkers::MarkerGeometry geo = DemodMarkers::markerGeometry(in);
        f.dl->AddRectFilled(ImVec2(f.nx(geo.label.x0), f.ny(geo.label.y1)),
            ImVec2(f.nx(geo.label.x1), f.ny(geo.label.y0)), packColor(geo.bg.r, geo.bg.g, geo.bg.b, geo.bg.a));
        ImU32 bandCol = packColor(color.r, color.g, color.b, 0.2f);
        f.dl->AddRectFilled(ImVec2(f.nx(geo.band.x0), f.ny(geo.band.y1)),
            ImVec2(f.nx(geo.band.x1), f.ny(geo.band.y0)), bandCol);
        if (geo.narrowBand) {
            f.dl->AddRectFilled(ImVec2(f.nx(geo.label.x0), f.ny(geo.label.y1)),
                ImVec2(f.nx(geo.label.x1), f.ny(geo.label.y0)), bandCol);
        }
        if (demod.selected) {
            float cx = f.nx(geo.uxPos);
            f.dl->AddLine(ImVec2(cx, f.ny(1.0f)), ImVec2(cx, f.ny(-1.0f)),
                packColor(color.r, color.g, color.b, 0.5f), 1.0f);
        }
        std::string demodLabel = geo.prefix;
        demodLabel.append(demod.label);
        drawTrackedText(f.dl, demodLabel.c_str(), f.nx(geo.uxPos), f.ny(geo.hPos), 16.0f,
            IM_COL32(255, 255, 255, 204), toTextAlign(geo.align));
    }

    if (hovered) {
        long long freq = WaterfallDrag::freqAtX(mx01, s.centerFreq, s.spanHz);
        if (s.snap > 1) {
            freq = (long long)(roundf((float)freq / (float)s.snap) * (float)s.snap);
        }
        const ThemeColor &color = s.ghostNew ? theme.waterfallNew : theme.waterfallHover;
        DemodMarkers::MarkerInput in;
        in.freqHz = freq;
        in.bandwidthHz = 200000;
        in.sideband = DemodMarkers::sidebandForType("FM");
        in.viewCenterHz = s.centerFreq;
        in.viewSpanHz = s.spanHz;
        in.viewWidthPx = viewW;
        in.viewHeightPx = viewH;
        in.stackLabels = true;
        DemodMarkers::MarkerGeometry geo = DemodMarkers::markerGeometry(in);
        f.dl->AddRectFilled(ImVec2(f.nx(geo.label.x0), f.ny(geo.label.y1)),
            ImVec2(f.nx(geo.label.x1), f.ny(geo.label.y0)), IM_COL32(0, 0, 0, 89));
        ImU32 bandCol = packColor(color.r, color.g, color.b, 0.1f);
        f.dl->AddRectFilled(ImVec2(f.nx(geo.band.x0), f.ny(geo.band.y1)),
            ImVec2(f.nx(geo.band.x1), f.ny(geo.band.y0)), bandCol);
        if (geo.narrowBand) {
            f.dl->AddRectFilled(ImVec2(f.nx(geo.label.x0), f.ny(geo.label.y1)),
                ImVec2(f.nx(geo.label.x1), f.ny(geo.label.y0)), bandCol);
        }
        float cx = f.nx(geo.uxPos);
        f.dl->AddLine(ImVec2(cx, f.ny(1.0f)), ImVec2(cx, f.ny(-1.0f)),
            packColor(color.r, color.g, color.b, 0.5f), 1.0f);
        float lx = f.nx(geo.uxPos);
        float ly = f.ny(geo.hPos);
        float dxPx = geo.shadowDx * wsize.x * 0.5f;
        float dyPx = geo.shadowDy * wsize.y * 0.5f;
        ImU32 mainCol = packColor(color.r, color.g, color.b);
        drawTrackedText(
            f.dl, geo.freqText.c_str(), lx + dxPx, ly + dyPx, 16.0f, IM_COL32(0, 0, 0, 255), toTextAlign(geo.align));
        drawTrackedText(
            f.dl, geo.freqText.c_str(), lx - dxPx, ly - dyPx, 16.0f, IM_COL32(0, 0, 0, 255), toTextAlign(geo.align));
        drawTrackedText(f.dl, geo.freqText.c_str(), lx, ly, 16.0f, mainCol, toTextAlign(geo.align));
    }

    if (s.showDb) {
        SpectrumTicks::DbLabels db =
            SpectrumTicks::dbLabels((double)s.ceilValue, (double)s.floorValue, s.fftSize, 0.0);
        ImU32 chipL = IM_COL32(51, 51, 51, 255);
        ImU32 chipR = IM_COL32(51, 51, 51, 0);
        float chipW = 88.0f;
        float chipH = 14.0f;
        f.dl->AddRectFilledMultiColor(ImVec2(f.ox, f.oy), ImVec2(f.ox + chipW, f.oy + chipH), chipL, chipR, chipR,
            chipL);
        drawTrackedText(f.dl, db.ceil.c_str(), f.ox + chipW * 0.5f, f.oy + chipH * 0.5f, 12.0f,
            IM_COL32_WHITE, TextAlign::Right);
        f.dl->AddRectFilledMultiColor(ImVec2(f.ox, f.oy + wsize.y - chipH), ImVec2(f.ox + chipW, f.oy + wsize.y),
            chipL, chipR, chipR, chipL);
        drawTrackedText(f.dl, db.floor.c_str(), f.ox + chipW * 0.5f, f.oy + wsize.y - chipH * 0.5f, 12.0f,
            IM_COL32_WHITE, TextAlign::Right);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void drawControl(AppState &s, int ww) {
    ImGui::SetNextWindowPos(ImVec2((float)ww - 340.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Spectrum spike");
    ImGui::Text("center %lld  span %lld", s.centerFreq, s.spanHz);
    ImGui::Text("gain %.2f%s  theme %s", (double)s.scaleFactor, s.resetScale ? " (resetting)" : "",
        kThemes[s.theme].name);
    ImGui::Text("dB %s  peak %s  ghost %s  snap %d", s.showDb ? "on" : "off", s.showPeak ? "on" : "off",
        s.ghostNew ? "new" : "hover", s.snap);
    ImGui::TextWrapped(
        "Drag to pan. Wheel to zoom. Right-drag for visual gain, right-click to reset. "
        "B toggles dB, P peak hold, T theme, N ghost color, S snap.");
    ImGui::End();
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    std::printf("spectrum via imgui draw lists; close the window to quit\n");
    std::fflush(stdout);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    const int startW = 1000;
    const int startH = 560;
    SDL_Window *window = SDL_CreateWindow("CubicSDR spectrum spike", startW, startH, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGL;
    init.reset = BGFX_RESET_VSYNC;
    init.swapChain.width = (uint32_t)startW;
    init.swapChain.height = (uint32_t)startH;
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
#if defined(_WIN32)
    init.swapChain.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__linux__)
    const char *driver = SDL_GetCurrentVideoDriver();
    if (driver && std::strcmp(driver, "wayland") == 0) {
        init.swapChain.ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        init.swapChain.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    } else {
        init.swapChain.ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        init.swapChain.nwh = (void *)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    }
#elif defined(__APPLE__)
    init.swapChain.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#endif
    bgfx::SwapChain swapChain = init.swapChain;
    if (!bgfx::init(init)) {
        std::printf("bgfx::init failed\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    std::printf("bgfx init ok: %s\n", bgfx::getRendererName(bgfx::getRendererType()));
    std::fflush(stdout);

    if (!imguiInitBackend()) {
        std::printf("imgui shader load failed\n");
        bgfx::shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    AppState s;
    initDemods(s);
    ImguiInput in;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float simT = 0.0f;
    uint64_t lastNs = SDL_GetTicksNS();
    bool running = true;
    while (running) {
        FrameEdges edges;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                swapChain.width = (uint32_t)e.window.data1;
                swapChain.height = (uint32_t)e.window.data2;
                bgfx::reset(BGFX_RESET_VSYNC, &swapChain);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                mouseX = e.motion.x;
                mouseY = e.motion.y;
                in.mx = mouseX;
                in.my = mouseY;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                bool down = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                mouseX = e.button.x;
                mouseY = e.button.y;
                in.mx = mouseX;
                in.my = mouseY;
                if (e.button.button == SDL_BUTTON_LEFT) {
                    in.left = down;
                    if (down) {
                        edges.leftPressed = true;
                    } else {
                        edges.leftReleased = true;
                    }
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    in.right = down;
                    if (down) {
                        edges.rightPressed = true;
                    } else {
                        edges.rightReleased = true;
                    }
                } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                    in.middle = down;
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
                edges.wheelRotV += (int)(e.wheel.y * 120.0f);
                in.wheelX += e.wheel.x;
                in.wheelY += e.wheel.y;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat) {
                    if (e.key.scancode == SDL_SCANCODE_B) {
                        edges.bPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_P) {
                        edges.pPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_T) {
                        edges.tPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_N) {
                        edges.nPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_S) {
                        edges.sPressed = true;
                    }
                }
                break;
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                edges.mouseLeft = true;
                in.mx = -FLT_MAX;
                in.my = -FLT_MAX;
                in.left = false;
                in.right = false;
                in.middle = false;
                break;
            default:
                break;
            }
        }

        uint64_t nowNs = SDL_GetTicksNS();
        float dt = (float)((nowNs - lastNs) / 1000000000.0);
        lastNs = nowNs;
        if (dt > 0.1f) {
            dt = 0.1f;
        }
        simT += dt;
        synthSpectrum(s, simT);
        int ww = startW;
        int wh = startH;
        SDL_GetWindowSize(window, &ww, &wh);

        imguiBeginFrame(in, ww, wh, dt);
        in.wheelX = 0.0f;
        in.wheelY = 0.0f;
        drawSpectrum(s, edges, mouseX, mouseY, ww, wh, (long long)SDL_GetTicks());
        drawControl(s, ww);
        imguiEndFrame(1);

        bgfx::frame();
    }

    imguiShutdownBackend();
    bgfx::frame();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
