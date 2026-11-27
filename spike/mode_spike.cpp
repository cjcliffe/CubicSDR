// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "imgui_bgfx.h"

#include <imgui.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#include <ModeSelector.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct ThemeColor {
    float r, g, b, a;
};

const ThemeColor kBackground{0.1f, 0.1f, 0.1f, 1.0f};
const ThemeColor kButton{0.65f, 0.65f, 0.65f, 1.0f};
const ThemeColor kButtonHighlight{1.0f, 1.0f, 0.0f, 1.0f};

struct Choice {
    int value;
    std::string label;
};

struct SelectorWidget {
    std::string name;
    float x, y, w, h;
    std::vector<Choice> choices;
    int selection = -1;
    bool toggleMode = false;
    float padX = 4.0f;
    float padY = 4.0f;
    bool highlightOverride = false;
    ThemeColor highlight{1.0f, 1.0f, 1.0f, 1.0f};
    bool pulseMode = false;
    float pulsePhase = 0.0f;
    bool disabled = false;
    bool changed = false;
    bool pressed = false;
    float originX = 0.0f;
    float lastStatus[2] = {-FLT_MAX, -FLT_MAX};
};

struct FrameEdges {
    bool leftPressed = false;
    bool leftReleased = false;
    bool mouseLeft = false;
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

int utf8CharBytes(const char *p, const char *end) {
    unsigned char c = (unsigned char)*p;
    int bytes = 1;
    if ((c & 0x80) == 0) {
        bytes = 1;
    } else if ((c & 0xE0) == 0xC0) {
        bytes = 2;
    } else if ((c & 0xF0) == 0xE0) {
        bytes = 3;
    } else if ((c & 0xF8) == 0xF0) {
        bytes = 4;
    }
    if (p + bytes > end) {
        bytes = (int)(end - p);
    }
    return bytes;
}

void drawCenterText(ImDrawList *dl, const char *text, float cxPx, float cyPx, float fontPx, ImU32 col) {
    ImGui::PushFont(imguiMonoFont(), fontPx);
    float adv = fontPx * 0.5f;
    const char *end = text + std::strlen(text);
    int count = 0;
    for (const char *p = text; p < end;) {
        int bytes = utf8CharBytes(p, end);
        count++;
        p += bytes;
    }
    float x = cxPx - adv * (float)count * 0.5f;
    float y = cyPx - fontPx * 0.5f;
    for (const char *p = text; p < end;) {
        int bytes = utf8CharBytes(p, end);
        dl->AddText(ImVec2(x, y), col, p, p + bytes);
        x += adv;
        p += bytes;
    }
    ImGui::PopFont();
}

void initWidgets(std::vector<SelectorWidget> &widgets) {
    SelectorWidget modem;
    modem.name = "Modem";
    modem.x = 10.0f;
    modem.y = 10.0f;
    modem.w = 110.0f;
    modem.h = 360.0f;
    const char *labels[] = {"FM", "FMS", "NBFM", "AM", "CW", "LSB", "USB", "DSB", "I/Q"};
    for (int i = 0; i < 9; i++) {
        modem.choices.push_back({i + 1, labels[i]});
    }
    modem.selection = 1;
    widgets.push_back(modem);

    const char *keys[] = {"P", "M", "S", "V", "R"};
    const ThemeColor colors[] = {
        {0.2f, 0.8f, 0.2f, 1.0f}, {0.8f, 0.2f, 0.2f, 1.0f}, {0.8f, 0.8f, 0.2f, 1.0f},
        {0.8f, 0.8f, 0.2f, 1.0f}, {0.2f, 0.8f, 0.2f, 1.0f},
    };
    for (int i = 0; i < 5; i++) {
        SelectorWidget button;
        button.name = keys[i];
        button.x = 130.0f;
        button.y = 10.0f + i * 38.0f;
        button.w = 60.0f;
        button.h = 30.0f;
        button.choices.push_back({1, keys[i]});
        button.toggleMode = true;
        button.padX = -1.0f;
        button.padY = -1.0f;
        button.highlightOverride = true;
        button.highlight = colors[i];
        if (i == 4) {
            button.pulseMode = true;
        }
        widgets.push_back(button);
    }

    SelectorWidget disabled;
    disabled.name = "Dimmed";
    disabled.x = 130.0f;
    disabled.y = 10.0f + 5 * 38.0f;
    disabled.w = 60.0f;
    disabled.h = 30.0f;
    disabled.choices.push_back({1, "X"});
    disabled.toggleMode = true;
    disabled.padX = -1.0f;
    disabled.padY = -1.0f;
    disabled.highlightOverride = true;
    disabled.highlight = {0.8f, 0.2f, 0.2f, 1.0f};
    disabled.disabled = true;
    widgets.push_back(disabled);
}

bool widgetHovered(const SelectorWidget &w, float mx, float my, bool mouseLeft) {
    return !mouseLeft && mx >= w.x && my >= w.y && mx < w.x + w.w && my < w.y + w.h;
}

void drawSelectorWidget(SelectorWidget &w, ImDrawList *dl, const FrameEdges &edges, float mx, float my,
    long long timeMs, std::string &status) {
    int numChoices = (int)w.choices.size();
    bool hovered = widgetHovered(w, mx, my, edges.mouseLeft);
    float my01 = hovered ? 1.0f - (my - w.y) / w.h : 0.0f;
    int yval = ModeSelector::hoveredRow(my01, numChoices, hovered);

    if (edges.mouseLeft) {
        w.pressed = false;
    }
    if (edges.leftPressed && hovered) {
        w.pressed = true;
        w.originX = mx;
    }
    if (edges.leftReleased && w.pressed) {
        w.pressed = false;
        bool isClick = ((mx - w.originX) / w.w) < 2.0f / w.h;
        ModeSelector::ReleaseResult result =
            ModeSelector::resolveRelease(w.selection, yval, w.toggleMode, isClick, w.disabled);
        if (result.changed) {
            w.changed = true;
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%s changed to %d", w.name.c_str(), result.selection);
            status = buf;
        }
        w.selection = result.selection;
    }
    if (w.pulseMode && w.selection != -1) {
        w.pulsePhase = ModeSelector::pulsePhaseAt(timeMs);
    }

    ImU32 bgCol = packColor(kBackground.r, kBackground.g, kBackground.b);
    dl->AddRectFilled(ImVec2(w.x, w.y), ImVec2(w.x + w.w, w.y + w.h), bgCol);
    for (int i = 0; i < numChoices; i++) {
        ModeSelector::SelectorCell cell =
            ModeSelector::selectorCell(i, numChoices, w.w, w.h, w.padX, w.padY);
        float x0 = w.x + (cell.outline[0] + 1.0f) * 0.5f * w.w;
        float y0 = w.y + (1.0f - cell.outline[1]) * 0.5f * w.h;
        float x1 = w.x + (cell.outline[4] + 1.0f) * 0.5f * w.w;
        float y1 = w.y + (1.0f - cell.outline[5]) * 0.5f * w.h;
        float rx0 = roundf(x0 < x1 ? x0 : x1);
        float ry0 = roundf(y0 < y1 ? y0 : y1);
        float rx1 = roundf(x0 < x1 ? x1 : x0);
        float ry1 = roundf(y0 < y1 ? y1 : y0);
        float labelY = w.y + (1.0f - cell.labelY) * 0.5f * w.h;
        float labelX = w.x + w.w * 0.5f;
        bool rowHovered = (yval == i && !w.highlightOverride && !w.disabled);
        if (rowHovered) {
            ImU32 hc = packColor(kButtonHighlight.r, kButtonHighlight.g, kButtonHighlight.b);
            dl->AddRectFilled(ImVec2(rx0, ry0), ImVec2(rx1, ry1), hc);
            drawCenterText(dl, w.choices[i].label.c_str(), labelX, labelY, (float)cell.fontSize,
                IM_COL32(0, 0, 0, 255));
        } else {
            ThemeColor hc = kButton;
            if (w.highlightOverride) {
                hc = w.highlight;
            }
            if (w.pulseMode && i == w.selection) {
                ImU32 off = packColor(hc.r, hc.g, hc.b);
                dl->AddRect(ImVec2(rx0, ry0), ImVec2(rx1, ry1), off, 0.0f, 0, 1.0f);
                drawCenterText(dl, w.choices[i].label.c_str(), labelX, labelY, (float)cell.fontSize, off);
                float blend = ModeSelector::pulseBlend(w.pulsePhase);
                ImU32 on = packColor(hc.r, hc.g, hc.b, blend);
                dl->AddRectFilled(ImVec2(rx0, ry0), ImVec2(rx1, ry1), on);
                drawCenterText(dl, w.choices[i].label.c_str(), labelX, labelY, (float)cell.fontSize,
                    packColor(0.0f, 0.0f, 0.0f, blend));
            } else {
                float alpha = 1.0f;
                if (w.disabled) {
                    hc = kButton;
                    alpha = 0.3f;
                }
                bool on = (i == w.selection);
                ImU32 col = packColor(hc.r, hc.g, hc.b, alpha);
                if (on) {
                    dl->AddRectFilled(ImVec2(rx0, ry0), ImVec2(rx1, ry1), col);
                    drawCenterText(dl, w.choices[i].label.c_str(), labelX, labelY,
                        (float)cell.fontSize, packColor(0.0f, 0.0f, 0.0f, alpha));
                } else {
                    dl->AddRect(ImVec2(rx0, ry0), ImVec2(rx1, ry1), col, 0.0f, 0, 1.0f);
                    drawCenterText(dl, w.choices[i].label.c_str(), labelX, labelY,
                        (float)cell.fontSize, col);
                }
            }
        }
    }
}

void drawModeSpike(std::vector<SelectorWidget> &widgets, const FrameEdges &edges, float mx, float my, int ww,
    int wh, long long timeMs, std::string &status) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)ww, (float)wh), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Mode", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    imguiSetClearColor(
        ((uint32_t)clampByte(kBackground.r) << 24) | ((uint32_t)clampByte(kBackground.g) << 16)
        | ((uint32_t)clampByte(kBackground.b) << 8) | 0xFFu);
    dl->AddRectFilled(
        ImVec2(origin.x, origin.y), ImVec2(origin.x + wsize.x, origin.y + wsize.y), IM_COL32(26, 26, 26, 255));
    for (auto &w : widgets) {
        SelectorWidget shifted = w;
        shifted.x += origin.x;
        shifted.y += origin.y;
        std::string widgetStatus;
        drawSelectorWidget(shifted, dl, edges, mx, my, timeMs, widgetStatus);
        w.selection = shifted.selection;
        w.pressed = shifted.pressed;
        w.originX = shifted.originX;
        w.pulsePhase = shifted.pulsePhase;
        if (shifted.changed) {
            w.changed = true;
            status = widgetStatus;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void drawControl(std::vector<SelectorWidget> &widgets, const std::string &status, int ww) {
    ImGui::SetNextWindowPos(ImVec2((float)ww - 340.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mode spike");
    for (auto &w : widgets) {
        ImGui::Text("%s sel=%d%s%s", w.name.c_str(), w.selection, w.changed ? " CHANGED" : "",
            w.disabled ? " disabled" : "");
    }
    ImGui::Text("last: %s", status.empty() ? "-" : status.c_str());
    ImGui::TextWrapped("Hover and click the rows and buttons. R pulses while selected. "
                       "D toggles the dimmed demo. C clears change flags.");
    ImGui::End();
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    std::printf("mode selectors via imgui draw lists; close the window to quit\n");
    std::fflush(stdout);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    const int startW = 560;
    const int startH = 400;
    SDL_Window *window = SDL_CreateWindow("CubicSDR mode spike", startW, startH, SDL_WINDOW_RESIZABLE);
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

    std::vector<SelectorWidget> widgets;
    initWidgets(widgets);
    std::string status;
    ImguiInput in;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
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
                } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                    in.middle = down;
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
                in.wheelX += e.wheel.x;
                in.wheelY += e.wheel.y;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat) {
                    if (e.key.scancode == SDL_SCANCODE_D) {
                        widgets.back().disabled = !widgets.back().disabled;
                    } else if (e.key.scancode == SDL_SCANCODE_C) {
                        for (auto &w : widgets) {
                            w.changed = false;
                        }
                        status.clear();
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

        int ww = startW;
        int wh = startH;
        SDL_GetWindowSize(window, &ww, &wh);

        imguiBeginFrame(in, ww, wh, 1.0f / 60.0f);
        in.wheelX = 0.0f;
        in.wheelY = 0.0f;
        drawModeSpike(widgets, edges, mouseX, mouseY, ww, wh, (long long)SDL_GetTicks(), status);
        drawControl(widgets, status, ww);
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
