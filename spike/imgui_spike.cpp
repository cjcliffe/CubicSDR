// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "imgui_bgfx.h"
#include "imgui_draw_util.h"

#include <imgui.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#include <TunerStep.h>
#include <TunerLayout.h>
#include <TuneRange.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

using namespace TunerStep;
using namespace TunerLayout;

namespace {

const float kFreqDP = -1.0f;
const float kFreqW = (1.0f / 3.0f) * 2.0f;
const float kBwDP = -1.0f + (2.25f / 3.0f);
const float kBwW = (1.0f / 4.0f) * 2.0f;
const float kCenterDP = -1.0f + (2.0f / 3.0f) * 2.0f;
const float kCenterW = (1.0f / 3.0f) * 2.0f;

const int kFreqCount = 11;
const int kBwCount = 7;

const long long kChannelizerRateMax = 500000;

const ThemeColor kBarLight = {0.2f, 0.2f, 0.9f};
const ThemeColor kBarDark = {0.0f, 0.0f, 0.35f};
const ThemeColor kBarUp = {1.0f, 139.0f / 255.0f, 96.0f / 255.0f};
const ThemeColor kBarDown = {148.0f / 255.0f, 148.0f / 255.0f, 1.0f};

struct BarGroup {
    int start;
    int end;
    int count;
    float dp;
    float w;
    bool light;
};

const BarGroup kBarGroups[] = {
    {1, 3, kFreqCount, kFreqDP, kFreqW, true},
    {4, 6, kFreqCount, kFreqDP, kFreqW, false},
    {7, 9, kFreqCount, kFreqDP, kFreqW, true},
    {10, 11, kFreqCount, kFreqDP, kFreqW, false},
    {1, 3, kBwCount, kBwDP, kBwW, true},
    {4, 6, kBwCount, kBwDP, kBwW, false},
    {7, 7, kBwCount, kBwDP, kBwW, true},
    {1, 3, kFreqCount, kCenterDP, kCenterW, true},
    {4, 6, kFreqCount, kCenterDP, kCenterW, false},
    {7, 9, kFreqCount, kCenterDP, kCenterW, true},
    {10, 11, kFreqCount, kCenterDP, kCenterW, false},
};

enum HoverState { HOVER_NONE, HOVER_FREQ, HOVER_BW, HOVER_PPM, HOVER_CENTER };

struct AppState {
    long long freq = 100000000;
    long long bw = 200000;
    long long center = 100000000;
    long long sampleRate = 2400000;
    int ppm = 0;
    int snap = 1;
    bool halfBand = false;
    bool altDown = false;
    bool shiftDown = false;
    bool top = false;
    bool bottom = false;
    HoverState hoverState = HOVER_NONE;
    HoverState downState = HOVER_NONE;
    int hoverIndex = 0;
    int downIndex = 0;
    bool leftDown = false;
    bool rightDown = false;
    bool dragging = false;
    bool tunerHovered = false;
    float dragAccum = 0.0f;
    float originMX = 0.0f;
    float originMY = 0.0f;
    float mouseMX = 0.0f;
    float mouseMY = 0.0f;
    int wheelAccumV = 0;
    int wheelAccumH = 0;
    bool showDemo = true;
};

void stepTuner(AppState &s, HoverState state, bool up, int digit, bool preventCarry, bool zeroOut) {
    if (state == HOVER_FREQ) {
        long long demodFreq = s.freq;
        long long diff = demodFreq >= s.center ? demodFreq - s.center : s.center - demodFreq;
        StepInput step;
        step.value = demodFreq;
        step.digit = digit;
        step.up = up;
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.bottom = s.bottom;
        demodFreq = stepDigit(step);
        demodFreq = pinDemodCenter(demodFreq, s.center, s.sampleRate);
        if (s.sampleRate / 2 < diff) {
            s.center = demodFreq;
        }
        s.freq = demodFreq;
    }
    if (state == HOVER_BW) {
        StepInput step;
        step.value = s.bw;
        step.digit = digit;
        step.up = up;
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.doubleAmount = s.halfBand;
        step.bottom = s.bottom;
        long long nbw = stepDigit(step);
        if (nbw > kChannelizerRateMax) {
            nbw = kChannelizerRateMax;
        }
        s.bw = nbw;
    }
    if (state == HOVER_CENTER) {
        StepInput step;
        step.value = s.center;
        step.digit = digit;
        step.up = up;
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.bottom = s.bottom;
        s.center = stepDigit(step);
    }
    if (state == HOVER_PPM) {
        StepInput step;
        step.value = s.ppm;
        step.digit = digit;
        step.up = up;
        step.preventCarry = preventCarry;
        step.bottom = s.bottom;
        int ppm = (int)stepDigit(step);
        if (ppm > 2000) {
            ppm = 2000;
        }
        if (ppm < -2000) {
            ppm = -2000;
        }
        s.ppm = ppm;
    }
}

using TunerFrame = SpecFrame;

const float kBgLift = 26.0f / 255.0f;

void drawBarQuads(const TunerFrame &f, const TunerBars &bars, const ThemeColor &color) {
    ImU32 edge = packColor(color.r * 0.5f + kBgLift, color.g * 0.5f + kBgLift, color.b * 0.5f + kBgLift);
    ImU32 full = packColor(color.r + kBgLift, color.g + kBgLift, color.b + kBgLift);
    if (bars.top.empty() && bars.bottom.empty()) {
        return;
    }
    float x0 = !bars.top.empty() ? bars.top[2] : bars.bottom[2];
    float x1 = !bars.top.empty() ? bars.top[0] : bars.bottom[0];
    emitVGradient(f, x0, x1, edge, full, !bars.top.empty(), !bars.bottom.empty());
}

void drawDragBar(const TunerFrame &f, float p1, float p2) {
    std::vector<float> q = dragBarQuads(p1, p2);
    float ndx0 = q[0] < q[2] ? q[0] : q[2];
    float ndx1 = q[0] < q[2] ? q[2] : q[0];
    ImU32 edge =
        packColor(kBarDark.r * 0.5f + kBgLift, kBarDark.g * 0.5f + kBgLift, kBarDark.b * 0.5f + kBgLift);
    ImU32 full = packColor(kBarDark.r + kBgLift, kBarDark.g + kBgLift, kBarDark.b + kBgLift);
    emitVGradient(f, ndx0, ndx1, edge, full, true, true);
    for (const BarGroup &g : kBarGroups) {
        TunerBars bars = tunerBarQuads(g.start, g.end, g.count, g.dp, g.w, true, true);
        float ox0 = ndx0 > bars.top[2] ? ndx0 : bars.top[2];
        float ox1 = ndx1 < bars.top[0] ? ndx1 : bars.top[0];
        if (ox1 > ox0) {
            const ThemeColor &under = g.light ? kBarLight : kBarDark;
            ImU32 sEdge = packColor(kBarDark.r * 0.5f + under.r * 0.5f + kBgLift,
                kBarDark.g * 0.5f + under.g * 0.5f + kBgLift, kBarDark.b * 0.5f + under.b * 0.5f + kBgLift);
            ImU32 sFull = packColor(
                kBarDark.r + under.r + kBgLift, kBarDark.g + under.g + kBgLift, kBarDark.b + under.b + kBgLift);
            emitVGradient(f, ox0, ox1, sEdge, sFull, true, true);
        }
    }
}

ThemeColor regionColorAt(int index, int count) {
    if (count == kBwCount) {
        return (index >= 4 && index <= 6) ? kBarDark : kBarLight;
    }
    return ((index >= 4 && index <= 6) || index >= 10) ? kBarDark : kBarLight;
}

void drawBarQuadsStacked(const TunerFrame &f, const TunerBars &bars, const ThemeColor &over, const ThemeColor &under) {
    ImU32 edge = packColor(over.r * 0.5f + under.r * 0.5f + kBgLift, over.g * 0.5f + under.g * 0.5f + kBgLift,
        over.b * 0.5f + under.b * 0.5f + kBgLift);
    ImU32 full =
        packColor(over.r + under.r + kBgLift, over.g + under.g + kBgLift, over.b + under.b + kBgLift);
    if (bars.top.empty() && bars.bottom.empty()) {
        return;
    }
    float x0 = !bars.top.empty() ? bars.top[2] : bars.bottom[2];
    float x1 = !bars.top.empty() ? bars.top[0] : bars.bottom[0];
    emitVGradient(f, x0, x1, edge, full, !bars.top.empty(), !bars.bottom.empty());
}

void drawTunerDigits(const TunerFrame &f, long long value, int count, float dp, float w, float fontScale) {
    std::string text = std::to_string(value);
    std::vector<TunerChar> chars = tunerChars(count, (int)text.size(), dp, w);
    ImGui::PushFont(imguiMonoFont(), 32.0f * fontScale);
    float midY = f.ny(0.0f);
    for (const TunerChar &ch : chars) {
        char glyph[2] = {text[(size_t)ch.charIndex], '\0'};
        ImVec2 sz = ImGui::CalcTextSize(glyph);
        f.dl->AddText(ImVec2(f.nx(ch.x) - sz.x * 0.5f, midY - sz.y * 0.5f), IM_COL32_WHITE, glyph);
    }
    ImGui::PopFont();
}

void drawTunerGrid(const TunerFrame &f, int count, float dp, float w) {
    std::vector<float> verts = tunerGridVerts(count, dp, w);
    ImU32 col = packColor(0.65f, 0.65f, 0.65f, 0.25f);
    for (size_t i = 0; i + 3 < verts.size(); i += 4) {
        f.dl->AddLineV(floorf(f.nx(verts[i])), f.ny(1.0f), f.ny(-1.0f), col);
    }
}

void drawSnapBox(const TunerFrame &f, int snapExp, float viewHeightPx) {
    std::vector<float> box = tunerDigitBox(snapExp, kFreqCount, kFreqDP, kFreqW, viewHeightPx);
    float xa = f.nx(box[0]);
    float xb = f.nx(box[4]);
    float ya = f.ny(box[1]);
    float yb = f.ny(box[5]);
    float x0 = floorf(xa < xb ? xa : xb);
    float x1 = floorf(xa < xb ? xb : xa);
    float y0 = floorf(ya < yb ? ya : yb);
    float y1 = floorf(ya < yb ? yb : ya);
    f.dl->AddRect(ImVec2(x0, y0), ImVec2(x1 + 1.0f, y1 + 1.0f), IM_COL32(255, 0, 0, 255), 0.0f, 0, 1.0f);
}

struct FrameEdges {
    bool leftPressed = false;
    bool leftReleased = false;
    bool rightPressed = false;
    bool rightReleased = false;
    int wheelRotV = 0;
    int wheelRotH = 0;
    bool hPressed = false;
    bool mouseLeft = false;
};

void updateHover(AppState &s) {
    if (!s.leftDown) {
        s.top = s.mouseMY >= 0.5f;
        s.bottom = s.mouseMY <= 0.5f;
    }
    int index = digitIndexAt(s.mouseMX, kFreqCount, kFreqDP, kFreqW);
    if (index > 0) {
        s.hoverIndex = index;
        s.hoverState = s.altDown ? HOVER_PPM : HOVER_FREQ;
    }
    if (index == 0) {
        index = digitIndexAt(s.mouseMX, kBwCount, kBwDP, kBwW);
        if (index > 0) {
            s.hoverIndex = index;
            s.hoverState = HOVER_BW;
        }
    }
    if (index == 0) {
        index = digitIndexAt(s.mouseMX, kFreqCount, kCenterDP, kCenterW);
        if (index > 0) {
            s.hoverIndex = index;
            s.hoverState = HOVER_CENTER;
        }
    }
    if (index == 0) {
        s.hoverIndex = 0;
        s.hoverState = HOVER_NONE;
    }
}

const char *hoverTip(HoverState state) {
    switch (state) {
    case HOVER_FREQ:
        return "Click, wheel or drag(left/right) a digit to change frequency; SPACE for direct input. Right click to set/clear snap. Hold ALT to change PPM. Hold SHIFT to disable carry. SHIFT-right click to Zero Right.";
    case HOVER_BW:
        return "Click, wheel or drag(left/right) a digit to change bandwidth; SPACE for direct input.  Hold SHIFT to disable carry. SHIFT-right click to Zero Right.";
    case HOVER_CENTER:
        return "Click, wheel or drag(left/right) a digit to change center frequency; SPACE for direct input.  Hold SHIFT to disable carry. SHIFT-right click to Zero Right.";
    case HOVER_PPM:
        return "Click, wheel or drag(left/right) a digit to change device PPM offset.  Hold SHIFT to disable carry.";
    case HOVER_NONE:
    default:
        return "";
    }
}

void drawTuner(AppState &s, const FrameEdges &edges, float mouseX, float mouseY) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(900.0f, 170.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Tuner", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    s.tunerHovered = ImGui::IsWindowHovered();
    s.mouseMX = (mouseX - origin.x) / size.x;
    s.mouseMY = 1.0f - (mouseY - origin.y) / size.y;

    if (edges.mouseLeft) {
        s.hoverIndex = 0;
        s.hoverState = HOVER_NONE;
        s.leftDown = false;
        s.rightDown = false;
        s.shiftDown = false;
        s.altDown = false;
    } else {
        updateHover(s);
    }

    if (edges.leftPressed && s.tunerHovered && !s.rightDown) {
        s.originMX = s.mouseMX;
        s.originMY = s.mouseMY;
        s.dragAccum = 0.0f;
        s.downIndex = s.hoverIndex;
        s.downState = s.hoverState;
        s.leftDown = true;
    }
    if (edges.rightPressed && s.tunerHovered && !s.leftDown) {
        s.downIndex = s.hoverIndex;
        s.downState = s.hoverState;
        s.rightDown = true;
    }

    if (s.tunerHovered && s.hoverState != HOVER_NONE && !s.leftDown && s.hoverIndex != 0) {
        if (edges.wheelRotV != 0) {
            WheelSteps wheel = wheelSteps(s.wheelAccumV, edges.wheelRotV, 120);
            s.wheelAccumV = wheel.rest;
            for (int i = 0, n = wheel.steps > 0 ? wheel.steps : -wheel.steps; i < n; i++) {
                stepTuner(s, s.hoverState, wheel.steps > 0, s.hoverIndex - 1, s.shiftDown, false);
            }
        }
        if (edges.wheelRotH != 0) {
            WheelSteps wheel = wheelSteps(s.wheelAccumH, edges.wheelRotH, 120);
            s.wheelAccumH = wheel.rest;
            for (int i = 0, n = wheel.steps > 0 ? wheel.steps : -wheel.steps; i < n; i++) {
                stepTuner(s, s.hoverState, wheel.steps < 0, s.hoverIndex - 1, s.shiftDown, false);
            }
        }
    }

    if (s.leftDown) {
        if (s.downState != HOVER_NONE) {
            s.dragAccum += 5.0f * (s.mouseMX - s.originMX);
            DragSteps drag = dragSteps(s.dragAccum);
            s.dragAccum = drag.rest;
            for (int i = 0; i < drag.up; i++) {
                stepTuner(s, s.downState, true, s.downIndex - 1, s.shiftDown, false);
                s.dragging = true;
            }
            for (int i = 0; i < drag.down; i++) {
                stepTuner(s, s.downState, false, s.downIndex - 1, s.shiftDown, false);
                s.dragging = true;
            }
        } else {
            s.dragAccum = 0.0f;
            s.dragging = false;
        }
    }

    if (edges.leftReleased) {
        if (s.tunerHovered && s.hoverState != HOVER_NONE && !s.dragging && s.downState == s.hoverState
            && s.downIndex == s.hoverIndex) {
            stepTuner(s, s.hoverState, s.top, s.hoverIndex - 1, s.shiftDown, false);
        }
        s.leftDown = false;
        s.dragging = false;
    }
    if (edges.rightReleased) {
        if (s.shiftDown) {
            if (s.tunerHovered && s.hoverState != HOVER_NONE && !s.dragging && s.downState == s.hoverState
                && s.downIndex == s.hoverIndex) {
                stepTuner(s, s.hoverState, s.top, s.hoverIndex - 1, false, true);
            }
        } else if (s.tunerHovered && s.hoverState == HOVER_FREQ) {
            if (s.hoverIndex == 1) {
                s.snap = 1;
            } else if (s.hoverIndex > 1 && s.hoverIndex < 8) {
                int exp = 1;
                for (int i = 1; i < s.hoverIndex; i++) {
                    exp *= 10;
                }
                s.snap = (s.snap == exp) ? 1 : exp;
            }
        }
        s.rightDown = false;
    }

    TunerFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, size.x, size.y};
    ThemeColor clr = s.top ? kBarUp : kBarDown;
    for (const BarGroup &g : kBarGroups) {
        drawBarQuads(
            f, tunerBarQuads(g.start, g.end, g.count, g.dp, g.w, true, true), g.light ? kBarLight : kBarDark);
    }
    if (s.leftDown) {
        drawDragBar(f, s.originMX, s.mouseMX);
    }

    if (s.hoverIndex > 0 && !s.leftDown) {
        switch (s.hoverState) {
        case HOVER_FREQ:
            drawBarQuadsStacked(f,
                tunerBarQuads(s.hoverIndex, s.hoverIndex, kFreqCount, kFreqDP, kFreqW, s.top, s.bottom), clr,
                regionColorAt(s.hoverIndex, kFreqCount));
            break;
        case HOVER_BW:
            drawBarQuadsStacked(f, tunerBarQuads(s.hoverIndex, s.hoverIndex, kBwCount, kBwDP, kBwW, s.top, s.bottom),
                clr, regionColorAt(s.hoverIndex, kBwCount));
            break;
        case HOVER_CENTER:
            drawBarQuadsStacked(f,
                tunerBarQuads(s.hoverIndex, s.hoverIndex, kFreqCount, kCenterDP, kCenterW, s.top, s.bottom), clr,
                regionColorAt(s.hoverIndex, kFreqCount));
            break;
        case HOVER_PPM:
            drawBarQuadsStacked(f,
                tunerBarQuads(s.hoverIndex, s.hoverIndex, kFreqCount, kFreqDP, kFreqW, s.top, s.bottom), clr,
                regionColorAt(s.hoverIndex, kFreqCount));
            break;
        case HOVER_NONE:
            break;
        }
    }

    int fontSize = tunerFontSize(size.x, size.y);
    float fontScale = (float)fontSize / 32.0f;
    if (s.altDown) {
        drawTunerDigits(f, s.ppm, kFreqCount, kFreqDP, kFreqW, fontScale);
        drawTunerGrid(f, kFreqCount, kFreqDP, kFreqW);
    } else {
        drawTunerDigits(f, s.freq, kFreqCount, kFreqDP, kFreqW, fontScale);
        drawTunerGrid(f, kFreqCount, kFreqDP, kFreqW);
        if (s.snap != 1) {
            drawSnapBox(f, (int)log10((double)s.snap), size.y);
        }
    }
    drawTunerDigits(f, s.halfBand ? s.bw / 2 : s.bw, kBwCount, kBwDP, kBwW, fontScale);
    drawTunerGrid(f, kBwCount, kBwDP, kBwW);
    drawTunerDigits(f, s.center, kFreqCount, kCenterDP, kCenterW, fontScale);
    drawTunerGrid(f, kFreqCount, kCenterDP, kCenterW);
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void drawControl(AppState &s) {
    ImGui::Begin("Tuner state");
    ImGui::Text("freq %lld  bw %lld  center %lld", s.freq, s.bw, s.center);
    ImGui::Text("ppm %d  snap %d  sr %lld", s.ppm, s.snap, s.sampleRate);
    const char *names[] = {"none", "freq", "bw", "ppm", "center"};
    ImGui::Text("hover %s [%d]  down %s [%d]%s", names[s.hoverState], s.hoverIndex, names[s.downState], s.downIndex,
        s.dragging ? " dragging" : "");
    ImGui::Checkbox("Half band (H)", &s.halfBand);
    ImGui::Checkbox("Demo window", &s.showDemo);
    ImGui::TextWrapped("%s", hoverTip(s.hoverState));
    ImGui::End();
    if (s.showDemo) {
        ImGui::ShowDemoWindow(&s.showDemo);
    }
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    std::printf("tuner via imgui draw lists; H toggles half band; close the window to quit\n");
    std::fflush(stdout);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    const int startW = 1000;
    const int startH = 700;
    SDL_Window *window = SDL_CreateWindow("CubicSDR imgui tuner spike", startW, startH, SDL_WINDOW_RESIZABLE);
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
    ImguiInput in;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
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
                edges.wheelRotH += (int)(e.wheel.x * 120.0f);
                in.wheelX += e.wheel.x;
                in.wheelY += e.wheel.y;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat && e.key.scancode == SDL_SCANCODE_H) {
                    edges.hPressed = true;
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

        SDL_Keymod mods = SDL_GetModState();
        if (!edges.mouseLeft) {
            s.shiftDown = (mods & SDL_KMOD_SHIFT) != 0;
            s.altDown = (mods & SDL_KMOD_ALT) != 0;
        }
        if (edges.hPressed) {
            s.halfBand = !s.halfBand;
        }

        uint64_t nowNs = SDL_GetTicksNS();
        float dt = (float)((nowNs - lastNs) / 1000000000.0);
        lastNs = nowNs;
        if (dt > 0.1f) {
            dt = 0.1f;
        }
        int ww = startW;
        int wh = startH;
        SDL_GetWindowSize(window, &ww, &wh);

        imguiBeginFrame(in, ww, wh, dt);
        in.wheelX = 0.0f;
        in.wheelY = 0.0f;
        drawTuner(s, edges, mouseX, mouseY);
        drawControl(s);
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
