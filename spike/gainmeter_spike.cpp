// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "imgui_bgfx.h"

#include <imgui.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#include <GainSlider.h>
#include <SquelchState.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

using namespace GainSlider;

namespace {

struct ThemeColor {
    float r, g, b;
};

const ThemeColor kBg = {0.1f, 0.1f, 0.1f};
const ThemeColor kMeterLevel = {0.1f, 0.75f, 0.1f};
const ThemeColor kMeterValue = {0.75f, 0.1f, 0.1f};
const ThemeColor kHoverGray = {0.4f, 0.4f, 0.4f};
const ThemeColor kBorderGray = {0.8f, 0.8f, 0.8f};
const ThemeColor kHiDark = {0.3f, 0.3f, 0.3f};
const ThemeColor kHiLight = {0.65f, 0.65f, 0.65f};

const float kBgLift = 26.0f / 255.0f;

struct GainStripState {
    std::string name;
    float low = 0.0f;
    float high = 0.0f;
    float value = 0.0f;
    float highlight = 0.0f;
    bool changed = false;
    bool highlightVisible = false;
};

struct MeterState {
    float level = 0.0f;
    float levelMin = 0.0f;
    float levelMax = 1.0f;
    float inputValue = 0.0f;
    float userInputValue = 0.0f;
    float defaultValue = 0.0f;
    float snapOffset = 6.0f;
    bool hasDefault = false;
    bool suppressNextRelease = false;
    bool showUserInput = true;
    bool pressedHere = false;
    bool rightPressedHere = false;
    bool hovered = false;
};

struct AppState {
    GainStripState gains[3];
    MeterState meters[2];
    int hoverGain = -1;
    bool leftDown = false;
    bool rightDown = false;
    bool autoApply = true;
    bool showDemo = false;
    double runTime = 0.0;
};

ImU32 packColor(float r, float g, float b, float a = 1.0f) {
    auto clampByte = [](float v) -> uint8_t {
        if (v < 0.0f) {
            v = 0.0f;
        }
        if (v > 1.0f) {
            v = 1.0f;
        }
        return (uint8_t)(v * 255.0f);
    };
    return IM_COL32(clampByte(r), clampByte(g), clampByte(b), clampByte(a));
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

struct FrameEdges {
    bool leftPressed = false;
    int leftClicks = 0;
    bool leftReleased = false;
    bool rightPressed = false;
    bool rightReleased = false;
    bool middlePressed = false;
    int wheelRotV = 0;
    int wheelRotH = 0;
    bool mouseLeft = false;
};

void gainSetLevel(AppState &s, float nx, float ny, float bgH, const GainStrip &strip) {
    for (int i = 0; i < 3; i++) {
        float midX = gainBarMidX(strip, i);
        float lx = (nx - midX) / strip.barWidth;
        float ly = ny / (float)bgH;
        if (lx >= -1.0f && lx <= 1.0f && ly >= -1.0f && ly <= 1.0f) {
            GainStripState &g = s.gains[i];
            g.value = clampGain(hitValue(ly, g.low, g.high), g.low, g.high);
            g.changed = true;
            break;
        }
    }
}

void drawGain(AppState &s, const FrameEdges &edges, float mouseX, float mouseY, bool &gainPressedHere) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f), ImGuiCond_Always);
    ImGui::Begin("Gain", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    bool hovered = ImGui::IsWindowHovered();
    float nx = (mouseX - origin.x) / size.x * 2.0f - 1.0f;
    float ny = 1.0f - (mouseY - origin.y) / size.y * 2.0f;

    GainStrip strip = gainStripLayout(3);
    LabelLayout layout = meterLabelLayout((double)size.y, 1.0, 18.0);
    float bgH = (float)layout.bgH;

    if (edges.mouseLeft) {
        s.hoverGain = -1;
        gainPressedHere = false;
        for (int i = 0; i < 3; i++) {
            s.gains[i].highlightVisible = false;
        }
    } else {
        s.hoverGain = -1;
        for (int i = 0; i < 3; i++) {
            float midX = gainBarMidX(strip, i);
            float lx = (nx - midX) / strip.barWidth;
            float ly = ny / bgH;
            if (hovered && lx >= -1.0f && lx <= 1.0f && ly >= -1.0f && ly <= 1.0f) {
                s.hoverGain = i;
                s.gains[i].highlight = clampGain(hitValue(ly, s.gains[i].low, s.gains[i].high), s.gains[i].low,
                    s.gains[i].high);
                s.gains[i].highlightVisible = true;
            } else {
                s.gains[i].highlightVisible = false;
            }
        }
    }

    if (edges.leftPressed && hovered) {
        gainPressedHere = true;
        s.leftDown = true;
    }
    if (s.leftDown && gainPressedHere && hovered) {
        gainSetLevel(s, nx, ny, bgH, strip);
    }
    if (edges.leftReleased) {
        gainPressedHere = false;
        s.leftDown = false;
    }

    if (hovered && s.hoverGain >= 0) {
        if (edges.wheelRotV != 0) {
            GainStripState &g = s.gains[s.hoverGain];
            g.value = clampGain(wheelGainStep(g.value, edges.wheelRotV, g.low, g.high), g.low, g.high);
            g.changed = true;
        }
        if (edges.wheelRotH != 0) {
            GainStripState &g = s.gains[s.hoverGain];
            g.value = clampGain(wheelGainStep(g.value, edges.wheelRotH, g.low, g.high), g.low, g.high);
            g.changed = true;
        }
    }

    PanelFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, size.x, size.y};
    ImU32 bgL = packColor(kBg.r, kBg.g, kBg.b);
    ImU32 bgR = packColor(kBg.r * 0.5f, kBg.g * 0.5f, kBg.b * 0.5f);
    f.dl->AddRectFilledMultiColor(ImVec2(f.px(-1.0f), f.py(1.0f)), ImVec2(f.px(1.0f), f.py(-1.0f)), bgL, bgR, bgR, bgL);

    float fontPx = labelFontSize((float)layout.valH * size.y);
    for (int i = 0; i < 3; i++) {
        GainStripState &g = s.gains[i];
        float midX = gainBarMidX(strip, i);
        float x0 = f.px(midX - strip.barWidth);
        float x1 = f.px(midX + strip.barWidth);
        float yTop = f.py(bgH);
        float yBot = f.py(-bgH);
        f.dl->AddRectFilledMultiColor(ImVec2(x0, yTop), ImVec2(x1, yBot), bgL, bgR, bgR, bgL);
        f.dl->AddRect(ImVec2(x0, yTop), ImVec2(x1, yBot), packColor(kBorderGray.r, kBorderGray.g, kBorderGray.b),
            0.0f, 0, 1.0f);

        float lvNorm = levelNorm(g.value, g.low, g.high);
        float lvTop = f.py(bgH * (-1.0f + 2.0f * lvNorm));
        ImU32 lvEdgeL = packColor(kMeterLevel.r * 0.5f + kBg.r, kMeterLevel.g * 0.5f + kBg.g,
            kMeterLevel.b * 0.5f + kBg.b);
        ImU32 lvMid = packColor(kMeterLevel.r + kBg.r * 0.75f, kMeterLevel.g + kBg.g * 0.75f,
            kMeterLevel.b + kBg.b * 0.75f);
        ImU32 lvEdgeR = packColor(kMeterLevel.r * 0.5f + kBg.r * 0.5f, kMeterLevel.g * 0.5f + kBg.g * 0.5f,
            kMeterLevel.b * 0.5f + kBg.b * 0.5f);
        emitHBar(f, x0 + 1.0f, x1 - 1.0f, lvTop, yBot - 1.0f, lvEdgeL, lvMid, lvEdgeR);

        if (g.highlightVisible) {
            float hiNorm = levelNorm(g.highlight, g.low, g.high);
            float loNorm = lvNorm < hiNorm ? lvNorm : hiNorm;
            float loTop = f.py(bgH * (-1.0f + 2.0f * loNorm));
            ImU32 hiEdgeL = packColor(kHiDark.r + kMeterLevel.r * 0.5f + kBg.r,
                kHiDark.g + kMeterLevel.g * 0.5f + kBg.g, kHiDark.b + kMeterLevel.b * 0.5f + kBg.b);
            ImU32 hiMid = packColor(kHiLight.r + kMeterLevel.r + kBg.r * 0.75f,
                kHiLight.g + kMeterLevel.g + kBg.g * 0.75f, kHiLight.b + kMeterLevel.b + kBg.b * 0.75f);
            ImU32 hiEdgeR = packColor(kHiDark.r + kMeterLevel.r * 0.5f + kBg.r * 0.5f,
                kHiDark.g + kMeterLevel.g * 0.5f + kBg.g * 0.5f, kHiDark.b + kMeterLevel.b * 0.5f + kBg.b * 0.5f);
            emitHBar(f, x0 + 1.0f, x1 - 1.0f, loTop, yBot - 1.0f, hiEdgeL, hiMid, hiEdgeR);
            if (hiNorm > loNorm) {
                float hiTop = f.py(bgH * (-1.0f + 2.0f * hiNorm));
                ImU32 gxEdgeL = packColor(
                    kHiDark.r + kBg.r, kHiDark.g + kBg.g, kHiDark.b + kBg.b);
                ImU32 gxMid = packColor(
                    kHiLight.r + kBg.r * 0.75f, kHiLight.g + kBg.g * 0.75f, kHiLight.b + kBg.b * 0.75f);
                ImU32 gxEdgeR = packColor(kHiDark.r + kBg.r * 0.5f, kHiDark.g + kBg.g * 0.5f,
                    kHiDark.b + kBg.b * 0.5f);
                emitHBar(f, x0 + 1.0f, x1 - 1.0f, hiTop, loTop, gxEdgeL, gxMid, gxEdgeR);
            }
        }

        float cx = f.px(midX);
        centerText(f, g.name.c_str(), cx, f.py((float)layout.labY), fontPx);
        centerText(f, gainLabelText(g.value).c_str(), cx, f.py((float)layout.valY), fontPx);
    }
    ImGui::End();
}

float clamp01(float v) {
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v > 1.0f) {
        return 1.0f;
    }
    return v;
}

struct MeterBand {
    float frac;
    ThemeColor color;
};

void meterWheelStep(MeterState &m, int rotation) {
    float span = m.levelMax - m.levelMin;
    float movement = 3.0f * (float)rotation;
    float currentValue = m.showUserInput ? m.userInputValue : m.level;
    currentValue = currentValue + ((movement / 100.0f) * (span / 100.0f));
    if (currentValue > m.levelMax) {
        currentValue = m.levelMax;
    }
    if (currentValue < m.levelMin) {
        currentValue = m.levelMin;
    }
    m.userInputValue = currentValue;
}

void drawMeter(AppState &s, int meterIndex, const char *title, float winX, const FrameEdges &edges, float mouseX,
    float mouseY) {
    ImGui::SetNextWindowPos(ImVec2(winX, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(170.0f, 420.0f), ImGuiCond_Always);
    ImGui::Begin(title, nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    MeterState &m = s.meters[meterIndex];
    m.hovered = ImGui::IsWindowHovered();
    float my01 = 1.0f - (mouseY - origin.y) / size.y;
    float span = m.levelMax - m.levelMin;

    if (edges.mouseLeft) {
        m.hovered = false;
        m.pressedHere = false;
        m.rightPressedHere = false;
    } else {
        if (edges.leftPressed && m.hovered) {
            if (edges.leftClicks >= 2) {
                if (m.hasDefault) {
                    m.userInputValue = m.defaultValue;
                    m.suppressNextRelease = true;
                }
            } else {
                m.userInputValue = my01 * span + m.levelMin;
                m.pressedHere = true;
                s.leftDown = true;
            }
        }
        if (s.leftDown && m.pressedHere && m.hovered) {
            m.userInputValue = my01 * span + m.levelMin;
        }
        if (edges.leftReleased) {
            if (m.suppressNextRelease) {
                m.suppressNextRelease = false;
            } else if (m.pressedHere && m.hovered) {
                m.userInputValue = my01 * span + m.levelMin;
            }
            m.pressedHere = false;
        }
        if (edges.rightPressed && m.hovered) {
            m.rightPressedHere = true;
            s.rightDown = true;
        }
        if (edges.rightReleased) {
            if (m.rightPressedHere && m.hovered && m.showUserInput) {
                m.userInputValue = snapSquelchAbove(m.level, m.levelMin, m.levelMax, m.snapOffset);
            }
            m.rightPressedHere = false;
            s.rightDown = false;
        }
        if (edges.middlePressed && m.hovered && m.hasDefault) {
            m.userInputValue = m.defaultValue;
        }
        if (m.hovered) {
            if (edges.wheelRotV != 0) {
                meterWheelStep(m, edges.wheelRotV);
            }
            if (edges.wheelRotH != 0) {
                meterWheelStep(m, edges.wheelRotH);
            }
        }
    }

    float levelPos = span > 0.0f ? (m.level - m.levelMin) / span : 0.0f;
    float inputPos = span > 0.0f ? (m.userInputValue - m.levelMin) / span : 0.0f;
    levelPos = clamp01(levelPos);
    inputPos = clamp01(inputPos);
    MeterBand bands[4];
    int bandCount = 0;
    bands[bandCount++] = {1.0f, kBg};
    if (m.hovered) {
        bands[bandCount++] = {my01, kHoverGray};
    }
    bands[bandCount++] = {levelPos, kMeterLevel};
    if (m.showUserInput) {
        bands[bandCount++] = {inputPos, kMeterValue};
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
    PanelFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, size.x, size.y};
    float prev = 0.0f;
    for (int i = 0; i < bandCount; i++) {
        float top = bands[i].frac;
        if (top > prev) {
            float er = kBgLift, eg = kBgLift, eb = kBgLift;
            float cr = kBgLift, cg = kBgLift, cb = kBgLift;
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
}

void drawControl(AppState &s, float appliedGains[3]) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 428.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Gain/meter state");
    for (int i = 0; i < 3; i++) {
        GainStripState &g = s.gains[i];
        ImGui::Text(
            "%s value %.1f applied %.1f%s", g.name.c_str(), g.value, appliedGains[i], g.changed ? " *" : "");
    }
    if (ImGui::Button("Apply gains")) {
        for (int i = 0; i < 3; i++) {
            appliedGains[i] = s.gains[i].value;
            s.gains[i].changed = false;
        }
    }
    for (int i = 0; i < 2; i++) {
        MeterState &m = s.meters[i];
        ImGui::Text("meter%d level %.2f user %.2f input %.2f%s", i, m.level, m.userInputValue, m.inputValue,
            (m.inputValue != m.userInputValue) ? " changed" : "");
    }
    ImGui::Checkbox("Auto-apply meter inputs", &s.autoApply);
    ImGui::Checkbox("Demo window", &s.showDemo);
    ImGui::End();
    if (s.showDemo) {
        ImGui::ShowDemoWindow(&s.showDemo);
    }
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    std::printf("gain strips plus meter sliders via imgui; close the window to quit\n");
    std::fflush(stdout);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    const int startW = 1000;
    const int startH = 700;
    SDL_Window *window = SDL_CreateWindow("CubicSDR gain/meter spike", startW, startH, SDL_WINDOW_RESIZABLE);
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

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1a1a1aff, 1.0f, 0);

    if (!imguiInit(1)) {
        std::printf("imgui shader load failed\n");
        bgfx::shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    AppState s;
    s.gains[0] = {"RF", 0.0f, 50.0f, 20.0f};
    s.gains[1] = {"IF", 0.0f, 40.0f, 15.0f};
    s.gains[2] = {"BB", 0.0f, 62.0f, 30.0f};
    s.meters[0].levelMin = -100.0f;
    s.meters[0].levelMax = 0.0f;
    s.meters[0].inputValue = -80.0f;
    s.meters[0].userInputValue = -80.0f;
    s.meters[0].defaultValue = -100.0f;
    s.meters[0].hasDefault = true;
    s.meters[0].showUserInput = true;
    s.meters[1].levelMin = 0.0f;
    s.meters[1].levelMax = 1.0f;
    s.meters[1].showUserInput = false;
    float appliedGains[3] = {s.gains[0].value, s.gains[1].value, s.gains[2].value};

    ImguiInput in;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool gainPressedHere = false;
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
                        edges.leftClicks = (int)e.button.clicks;
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
                    if (down) {
                        edges.middlePressed = true;
                    }
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
                edges.wheelRotV += (int)(e.wheel.y * 120.0f);
                edges.wheelRotH += (int)(e.wheel.x * 120.0f);
                in.wheelX += e.wheel.x;
                in.wheelY += e.wheel.y;
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
        int ww = startW;
        int wh = startH;
        SDL_GetWindowSize(window, &ww, &wh);

        s.runTime += dt;
        s.meters[0].level =
            -70.0f + 15.0f * (float)sin(s.runTime * 1.9) + 5.0f * (float)sin(s.runTime * 10.7);
        s.meters[1].level =
            0.5f + 0.3f * (float)sin(s.runTime * 3.1 + 1.0) + 0.1f * (float)sin(s.runTime * 14.4);
        if (s.autoApply) {
            for (int i = 0; i < 2; i++) {
                s.meters[i].inputValue = s.meters[i].userInputValue;
            }
        }

        imguiBeginFrame(in, ww, wh, dt);
        in.wheelX = 0.0f;
        in.wheelY = 0.0f;
        drawGain(s, edges, mouseX, mouseY, gainPressedHere);
        drawMeter(s, 0, "Squelch", 628.0f, edges, mouseX, mouseY);
        drawMeter(s, 1, "Signal", 806.0f, edges, mouseX, mouseY);
        drawControl(s, appliedGains);
        imguiEndFrame();

        bgfx::setViewRect(0, 0, 0, (uint16_t)ww, (uint16_t)wh);
        bgfx::touch(0);
        bgfx::frame();
    }

    imguiShutdown();
    bgfx::frame();
    bgfx::shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
