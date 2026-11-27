// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "imgui_bgfx.h"

#include <imgui.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#include <DemodMarkers.h>
#include <TuneRange.h>
#include <WaterfallDrag.h>
#include <WaterfallHover.h>
#include <WaterfallPacer.h>
#include <WaterfallRing.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace {

struct LutStop {
    float r, g, b;
};

const LutStop kTurboStops[] = {
    {0.19f, 0.07f, 0.23f}, {0.24f, 0.21f, 0.55f}, {0.27f, 0.45f, 0.92f}, {0.22f, 0.65f, 0.98f},
    {0.27f, 0.97f, 0.53f}, {0.75f, 0.96f, 0.20f}, {0.98f, 0.73f, 0.22f}, {0.48f, 0.02f, 0.01f},
};

const LutStop kClassicStops[] = {
    {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
    {1.0f, 0.2f, 0.0f}, {1.0f, 1.0f, 1.0f},
};

struct FakeDemod {
    long long freq;
    int bw;
    const char *type;
    const char *userLabel;
    bool selected = false;
};

struct AppState {
    long long centerFreq = 100000000;
    long long spanHz = 2000000;
    int lps = 30;
    int theme = 0;
    bool altDown = false;
    bool shiftDown = false;
    bool leftDown = false;
    float originX01 = 0.0f;
    float originY01 = 0.0f;
    float hoverAlpha = 1.0f;
    int hoverIndex = -1;
    int nextZone = 0;
    int dragState = 0;
    long long dragOfs = 0;
    int dragBW = 0;
    float lastDragX01 = 0.0f;
    int snap = 1;
    const char *lastType = "FM";
    WaterfallRing ring;
    WaterfallPacer pacer;
    std::deque<std::vector<float>> pending;
    bgfx::TextureHandle tex[2] = {BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE};
    std::vector<unsigned char> shadow[2];
    std::vector<uint8_t> lut;
    std::vector<uint8_t> staging;
    std::vector<FakeDemod> demods;
};

struct FrameEdges {
    bool leftPressed = false;
    bool leftReleased = false;
    int wheelRotV = 0;
    bool mouseLeft = false;
    bool tPressed = false;
    bool plusPressed = false;
    bool minusPressed = false;
    bool sPressed = false;
    bool yPressed = false;
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

struct SpecFrame {
    ImDrawList *dl;
    float ox, oy, w, h;
    float nx(float ndcX) const {
        return ox + (ndcX + 1.0f) * 0.5f * w;
    }
    float ny(float ndcY) const {
        return oy + (1.0f - ndcY) * 0.5f * h;
    }
};

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

void drawTrackedText(const SpecFrame &f, const char *text, float xPx, float cyPx, float fontPx, ImU32 col) {
    ImGui::PushFont(imguiMonoFont(), fontPx);
    float adv = fontPx * 0.5f;
    const char *end = text + std::strlen(text);
    int count = 0;
    for (const char *p = text; p < end;) {
        int bytes = utf8CharBytes(p, end);
        count++;
        p += bytes;
    }
    float x = xPx - adv * (float)count * 0.5f;
    float y = cyPx - fontPx * 0.5f;
    for (const char *p = text; p < end;) {
        int bytes = utf8CharBytes(p, end);
        f.dl->AddText(ImVec2(x, y), col, p, p + bytes);
        x += adv;
        p += bytes;
    }
    ImGui::PopFont();
}

void buildLut(AppState &s) {
    const LutStop *stops = s.theme == 0 ? kTurboStops : kClassicStops;
    int nStops = s.theme == 0 ? 8 : 6;
    s.lut.resize(256 * 4);
    for (int i = 0; i < 256; i++) {
        float u = (float)i / 255.0f * (float)(nStops - 1);
        int s0 = (int)u;
        if (s0 >= nStops - 1) {
            s0 = nStops - 2;
        }
        float f = u - (float)s0;
        s.lut[i * 4] = clampByte(stops[s0].r + (stops[s0 + 1].r - stops[s0].r) * f);
        s.lut[i * 4 + 1] = clampByte(stops[s0].g + (stops[s0 + 1].g - stops[s0].g) * f);
        s.lut[i * 4 + 2] = clampByte(stops[s0].b + (stops[s0 + 1].b - stops[s0].b) * f);
        s.lut[i * 4 + 3] = 255;
    }
}

void initDemods(AppState &s) {
    s.demods = {
        {100100000, 200000, "FM", "JAZZ", true},
        {99950000, 3000, "USB", "", false},
        {100300000, 3000, "LSB", "", false},
        {100020000, 5000, "AM", "", false},
    };
}

std::vector<float> synthLine(float t) {
    const int n = 2048;
    std::vector<float> line(n);
    float burst = expf(-powf(fmodf(t * 0.11f, 1.0f) - 0.5f, 2.0f) / 0.002f);
    for (int i = 0; i < n; i++) {
        float x = (float)i / (float)n;
        float v = 0.08f + 0.05f * sinf(x * 173.0f + t * 4.1f) * sinf(x * 61.0f - t * 2.3f);
        float dx0 = (x - (0.30f + 0.02f * sinf(t * 0.4f))) / 0.012f;
        float dx1 = (x - (0.55f + 0.015f * sinf(t * 0.23f))) / 0.006f;
        float dx2 = (x - 0.72f) / 0.03f;
        v += 0.6f * expf(-dx0 * dx0);
        v += 0.8f * expf(-dx1 * dx1);
        v += (0.3f + 0.5f * burst) * expf(-dx2 * dx2);
        line[i] = v;
    }
    return line;
}

void expandRows(AppState &s, const unsigned char *bytes, int halfW, int rows) {
    s.staging.resize((size_t)halfW * (size_t)rows * 4);
    for (int i = 0; i < halfW * rows; i++) {
        const uint8_t *entry = &s.lut[(size_t)bytes[i] * 4];
        s.staging[(size_t)i * 4] = entry[0];
        s.staging[(size_t)i * 4 + 1] = entry[1];
        s.staging[(size_t)i * 4 + 2] = entry[2];
        s.staging[(size_t)i * 4 + 3] = 255;
    }
}

void feedWaterfall(AppState &s, double dt, float t) {
    s.pending.push_back(synthLine(t));
    while (s.pending.size() > 120) {
        s.pending.pop_front();
    }
    WaterfallPacer::Plan plan = s.pacer.plan(dt, s.pending.size());
    for (int d = 0; d < plan.dropBefore && !s.pending.empty(); d++) {
        s.pending.pop_front();
    }
    bool updated = false;
    for (int i = 0; i < plan.insert && !s.pending.empty(); i++) {
        s.ring.setPoints(s.pending.front());
        s.pending.pop_front();
        s.ring.step();
        updated = true;
    }
    for (int d = 0; d < plan.dropAfter && !s.pending.empty(); d++) {
        s.pending.pop_front();
    }
    if (!updated) {
        return;
    }
    int halfW = 1024;
    WaterfallRing::UploadPlan up = s.ring.planUpload();
    for (const auto &op : up.ops) {
        const unsigned char *src = s.ring.reversedLine(op.texture) + op.srcOffset;
        std::memcpy(&s.shadow[op.texture][(size_t)op.y * (size_t)halfW], src,
            (size_t)op.height * (size_t)halfW);
        expandRows(s, src, halfW, op.height);
        const bgfx::Memory *mem = bgfx::copy(s.staging.data(), (uint32_t)s.staging.size());
        bgfx::updateTexture2D(s.tex[op.texture], 0, 0, 0, (uint16_t)op.y, (uint16_t)halfW,
            (uint16_t)op.height, mem);
    }
    s.ring.commitUpload(up);
}

void reexpandAll(AppState &s, int halfW, int lines) {
    for (int t = 0; t < 2; t++) {
        expandRows(s, s.shadow[t].data(), halfW, lines);
        const bgfx::Memory *mem = bgfx::copy(s.staging.data(), (uint32_t)s.staging.size());
        bgfx::updateTexture2D(s.tex[t], 0, 0, 0, 0, (uint16_t)halfW, (uint16_t)lines, mem);
    }
}

void drawDemodEdges(const SpecFrame &f, AppState &s, const FakeDemod &demod, int viewW, int viewH,
    ImU32 edgeCol, float washAlpha) {
    DemodMarkers::MarkerInput in;
    in.freqHz = demod.freq;
    in.bandwidthHz = demod.bw;
    in.sideband = DemodMarkers::sidebandForType(demod.type);
    in.viewCenterHz = s.centerFreq;
    in.viewSpanHz = s.spanHz;
    in.viewWidthPx = viewW;
    in.viewHeightPx = viewH;
    DemodMarkers::MarkerGeometry geo = DemodMarkers::markerGeometry(in);
    float cx = f.nx(geo.uxPos);
    f.dl->PushTexture(imguiAdditiveTexture());
    f.dl->AddLine(ImVec2(cx, f.ny(1.0f)), ImVec2(cx, f.ny(-1.0f)), edgeCol, 1.0f);
    float lx = f.nx(geo.uxPos - geo.ofsLeft);
    f.dl->AddLine(ImVec2(lx, f.ny(1.0f)), ImVec2(lx, f.ny(-1.0f)), edgeCol, 1.0f);
    float rx = f.nx(geo.uxPos + geo.ofsRight);
    f.dl->AddLine(ImVec2(rx, f.ny(1.0f)), ImVec2(rx, f.ny(-1.0f)), edgeCol, 1.0f);
    if (washAlpha > 0.0f) {
        ImU32 washCol = (edgeCol & 0x00FFFFFF) | ((ImU32)clampByte(0.2f * washAlpha) << 24);
        f.dl->AddRectFilled(ImVec2(f.nx(geo.uxPos - geo.ofsLeft), f.ny(1.0f)),
            ImVec2(f.nx(geo.uxPos + geo.ofsRight), f.ny(-1.0f)), washCol);
    }
    f.dl->PopTexture();
    float uxUnit = (float)(demod.freq - (s.centerFreq - s.spanHz / 2)) / (float)s.spanHz;
    float xOfs = 2.0f / (float)viewW;
    std::string typeLabel = DemodMarkers::demodTypeLabel(demod.type, false);
    if (typeLabel == "LSB") {
        uxUnit -= xOfs;
    } else if (typeLabel == "USB") {
        uxUnit += xOfs;
    }
    float hPos = 20.0f / (float)viewH;
    float labelX = f.nx(2.0f * (uxUnit - 0.5f));
    if (demod.userLabel[0] != '\0') {
        float typeY = f.ny(-1.0f + hPos * 1.2f + hPos * 1.2f);
        float userY = f.ny(-1.0f + hPos * 1.2f);
        drawTrackedText(f, typeLabel.c_str(), labelX + 1.0f, typeY + 1.0f, 16.0f, IM_COL32(0, 0, 0, 255));
        drawTrackedText(f, typeLabel.c_str(), labelX, typeY, 16.0f, IM_COL32_WHITE);
        drawTrackedText(f, demod.userLabel, labelX + 1.0f, userY + 1.0f, 16.0f, IM_COL32(0, 0, 0, 255));
        drawTrackedText(f, demod.userLabel, labelX, userY, 16.0f, IM_COL32_WHITE);
    } else {
        float labelY = f.ny(-1.0f + hPos * 1.2f);
        drawTrackedText(f, typeLabel.c_str(), labelX + 1.0f, labelY + 1.0f, 16.0f, IM_COL32(0, 0, 0, 255));
        drawTrackedText(f, typeLabel.c_str(), labelX, labelY, 16.0f, IM_COL32_WHITE);
    }
}

int pickHoverDemod(AppState &s, float mx01, int &zone) {
    long long freqPos = WaterfallDrag::freqAtX(mx01, s.centerFreq, s.spanHz);
    std::vector<WaterfallHover::HoverCandidate> candidates;
    std::vector<int> index;
    for (size_t i = 0; i < s.demods.size(); i++) {
        long long halfBw = (long long)s.demods[i].bw / 2;
        bool isLSB = std::strcmp(s.demods[i].type, "LSB") == 0;
        bool isUSB = std::strcmp(s.demods[i].type, "USB") == 0;
        long long lo = s.demods[i].freq - (isUSB ? 0 : halfBw) - 7500;
        long long hi = s.demods[i].freq + (isLSB ? 0 : halfBw) + 7500;
        if (freqPos < lo || freqPos > hi) {
            continue;
        }
        WaterfallHover::HoverCandidate cand;
        cand.freqHz = s.demods[i].freq;
        cand.bandwidthHz = s.demods[i].bw;
        cand.sideband = DemodMarkers::sidebandForType(s.demods[i].type);
        candidates.push_back(cand);
        index.push_back((int)i);
    }
    if (candidates.empty() || s.shiftDown) {
        zone = 0;
        return -1;
    }
    WaterfallHover::HoverPick pick = WaterfallHover::pickHover(candidates, freqPos, s.spanHz, 2400000);
    if (pick.zone == WaterfallHover::DragZone::None) {
        zone = 0;
        return -1;
    }
    if (pick.zone != WaterfallHover::DragZone::KeepPrevious) {
        zone = (int)pick.zone;
    }
    return index[(size_t)pick.index];
}

void drawWaterfall(AppState &s, const FrameEdges &edges, float mx, float my, int ww, int wh) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)ww, (float)wh), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Waterfall", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    SpecFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, wsize.x, wsize.y};
    bool hovered = !edges.mouseLeft && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    float mx01 = (mx - origin.x) / wsize.x;

    if (edges.mouseLeft) {
        s.leftDown = false;
        s.hoverIndex = -1;
        s.hoverAlpha = 0.0f;
    }
    if (edges.tPressed) {
        s.theme = (s.theme + 1) % 2;
        buildLut(s);
        reexpandAll(s, 1024, 512);
    }
    if (edges.plusPressed) {
        s.lps += 5;
        if (s.lps > 120) {
            s.lps = 120;
        }
        s.pacer.setLinesPerSecond(s.lps);
    }
    if (edges.minusPressed) {
        s.lps -= 5;
        if (s.lps < 5) {
            s.lps = 5;
        }
        s.pacer.setLinesPerSecond(s.lps);
    }
    if (edges.sPressed) {
        s.snap = s.snap > 1 ? 1 : 12500;
    }
    if (edges.yPressed) {
        if (std::strcmp(s.lastType, "FM") == 0) {
            s.lastType = "USB";
        } else if (std::strcmp(s.lastType, "USB") == 0) {
            s.lastType = "LSB";
        } else {
            s.lastType = "FM";
        }
    }
    if (edges.leftPressed && hovered) {
        s.leftDown = true;
        s.originX01 = mx01;
        s.originY01 = 1.0f - (my - origin.y) / wsize.y;
        s.dragState = 0;
        if (!s.altDown && s.hoverIndex >= 0) {
            if (s.nextZone == 1) {
                s.dragState = 3;
            } else if (s.nextZone == 2) {
                s.dragState = 1;
            } else if (s.nextZone == 3) {
                s.dragState = 2;
            }
            if (s.dragState != 0) {
                const FakeDemod &demod = s.demods[(size_t)s.hoverIndex];
                s.dragOfs = WaterfallDrag::dragOffset(mx01, s.spanHz, s.centerFreq, demod.freq);
                s.dragBW = demod.bw;
                s.lastDragX01 = mx01;
            }
        }
    }
    if (s.leftDown && !s.altDown && s.hoverIndex >= 0) {
        FakeDemod &demod = s.demods[(size_t)s.hoverIndex];
        if (s.dragState == 3) {
            long long target = WaterfallDrag::freqAtX(mx01, s.centerFreq, s.spanHz) - s.dragOfs;
            long long diff = WaterfallDrag::snapDelta(target - demod.freq, s.snap);
            if (diff != 0) {
                demod.freq = pinDemodCenter(demod.freq + diff, s.centerFreq, s.spanHz);
            }
        } else if (s.dragState == 1 || s.dragState == 2) {
            float dx = mx01 - s.lastDragX01;
            s.dragBW = WaterfallDrag::bandwidthDragStep(
                dx, s.spanHz, s.dragState == 1, s.dragBW, 500, 500000);
            demod.bw = s.dragBW;
            s.lastDragX01 = mx01;
        }
    }
    if (edges.leftReleased) {
        if (!s.altDown && (mx01 - s.originX01) == 0.0f
            && ((1.0f - (my - origin.y) / wsize.y) - s.originY01) == 0.0f) {
            if (s.dragState == 0) {
                for (size_t i = 0; i < s.demods.size(); i++) {
                    if (s.demods[i].selected) {
                        s.demods[i].freq = WaterfallDrag::clickPlaceFreq(
                            mx01, s.centerFreq, s.spanHz, true, s.demods[i].freq, s.snap);
                    }
                }
            } else if (s.hoverIndex >= 0) {
                for (size_t i = 0; i < s.demods.size(); i++) {
                    s.demods[i].selected = ((int)i == s.hoverIndex);
                }
            }
        }
        s.leftDown = false;
        s.dragState = 0;
    }
    if (edges.wheelRotV != 0 && hovered) {
        s.spanHz = (long long)((double)s.spanHz * (double)WaterfallDrag::wheelZoom(edges.wheelRotV, 120));
        if (s.spanHz < 30000) {
            s.spanHz = 30000;
        }
        if (s.spanHz > 12000000) {
            s.spanHz = 12000000;
        }
    }
    if (hovered && !s.leftDown) {
        s.hoverIndex = pickHoverDemod(s, mx01, s.nextZone);
    }
    if (hovered || s.hoverIndex >= 0) {
        s.hoverAlpha += (1.0f - s.hoverAlpha) * 0.1f;
        if (s.hoverAlpha > 1.5f) {
            s.hoverAlpha = 1.5f;
        }
    } else {
        s.hoverAlpha += (0.0f - s.hoverAlpha) * 0.05f;
        if (s.hoverAlpha < 1.0e-5f) {
            s.hoverAlpha = 0.0f;
        }
    }

    imguiSetClearColor(0x000000FF);
    int halfW = 1024;
    int lines = 512;
    int offset = s.ring.rowOffset();
    float u0 = 0.5f / (float)halfW;
    float u1 = 1.0f - 0.5f / (float)halfW;
    float splitY = f.oy + wsize.y * (float)(lines - offset) / (float)lines;
    float vTopA = ((float)offset + 0.5f) / (float)lines;
    float vBotA = ((float)lines - 0.5f) / (float)lines;
    ImTextureID texId0 = imguiTextureId(s.tex[0]);
    ImTextureID texId1 = imguiTextureId(s.tex[1]);
    f.dl->AddImage(texId0, ImVec2(f.ox, f.oy), ImVec2(f.ox + wsize.x * 0.5f + 0.5f, splitY),
        ImVec2(u0, vTopA), ImVec2(u1, vBotA));
    f.dl->AddImage(texId1, ImVec2(f.ox + wsize.x * 0.5f - 0.5f, f.oy), ImVec2(f.ox + wsize.x, splitY),
        ImVec2(u0, vTopA), ImVec2(u1, vBotA));
    if (offset > 0) {
        float vTopB = 0.5f / (float)lines;
        float vBotB = ((float)offset - 0.5f) / (float)lines;
        f.dl->AddImage(texId0, ImVec2(f.ox, splitY), ImVec2(f.ox + wsize.x * 0.5f + 0.5f, f.oy + wsize.y),
            ImVec2(u0, vTopB), ImVec2(u1, vBotB));
        f.dl->AddImage(texId1, ImVec2(f.ox + wsize.x * 0.5f - 0.5f, splitY), ImVec2(f.ox + wsize.x, f.oy + wsize.y),
            ImVec2(u0, vTopB), ImVec2(u1, vBotB));
    }

    int viewW = (int)wsize.x;
    int viewH = (int)wsize.y;
    bool isNew = s.shiftDown;
    bool dimSelection = s.hoverIndex >= 0 && !s.demods[(size_t)s.hoverIndex].selected;
    for (size_t i = 0; i < s.demods.size(); i++) {
        const FakeDemod &demod = s.demods[i];
        bool selected = demod.selected;
        if ((int)i == s.hoverIndex || (selected && !dimSelection)) {
            continue;
        }
        drawDemodEdges(f, s, demod, viewW, viewH, packColor(1.0f, 1.0f, 1.0f, 0.51f), 0.0f);
    }
    if (hovered || s.hoverIndex >= 0) {
        int selectedIdx = -1;
        for (size_t i = 0; i < s.demods.size(); i++) {
            if (s.demods[i].selected) {
                selectedIdx = (int)i;
            }
        }
        if (s.altDown) {
            float width = 1.0f / wsize.x;
            float rangeWidth = mx01 - s.originX01;
            float centerPos;
            if (s.leftDown) {
                if (rangeWidth != 0.0f) {
                    width = rangeWidth;
                }
                centerPos = s.originX01 + width / 2.0f;
            } else {
                centerPos = mx01;
            }
            if (selectedIdx >= 0) {
                drawDemodEdges(f, s, s.demods[(size_t)selectedIdx], viewW, viewH,
                    packColor(1.0f, 1.0f, 1.0f, 0.51f), 0.0f);
            }
            ImU32 rangeCol =
                isNew ? packColor(0.0f, 1.0f, 0.0f, 0.51f) : packColor(1.0f, 1.0f, 0.0f, 0.51f);
            bool singleSide =
                std::strcmp(s.lastType, "LSB") == 0 || std::strcmp(s.lastType, "USB") == 0;
            if (singleSide && s.leftDown) {
                centerPos = mx01;
                float p1 = centerPos;
                float p2 = centerPos - width;
                if (p2 < p1) {
                    float tmp = p1;
                    p1 = p2;
                    p2 = tmp;
                }
                float w1 = std::strcmp(s.lastType, "USB") == 0 ? 2.0f : 1.0f;
                float w2 = std::strcmp(s.lastType, "LSB") == 0 ? 2.0f : 1.0f;
                f.dl->PushTexture(imguiAdditiveTexture());
                f.dl->AddLine(ImVec2(f.nx((p1 - 0.5f) * 2.0f), f.ny(1.0f)),
                    ImVec2(f.nx((p1 - 0.5f) * 2.0f), f.ny(-1.0f)), rangeCol, w1);
                f.dl->AddLine(ImVec2(f.nx((p2 - 0.5f) * 2.0f), f.ny(1.0f)),
                    ImVec2(f.nx((p2 - 0.5f) * 2.0f), f.ny(-1.0f)), rangeCol, w2);
                f.dl->PopTexture();
            } else {
                float centerNDC = (centerPos - 0.5f) * 2.0f;
                f.dl->PushTexture(imguiAdditiveTexture());
                f.dl->AddLine(ImVec2(f.nx(centerNDC), f.ny(1.0f)), ImVec2(f.nx(centerNDC), f.ny(-1.0f)),
                    rangeCol, 1.0f);
                f.dl->AddLine(ImVec2(f.nx(centerNDC - width), f.ny(1.0f)),
                    ImVec2(f.nx(centerNDC - width), f.ny(-1.0f)), rangeCol, 1.0f);
                f.dl->AddLine(ImVec2(f.nx(centerNDC + width), f.ny(1.0f)),
                    ImVec2(f.nx(centerNDC + width), f.ny(-1.0f)), rangeCol, 1.0f);
                f.dl->PopTexture();
            }
        } else {
            if (selectedIdx >= 0 && !dimSelection) {
                bool lit = (isNew && s.hoverIndex < 0) || s.hoverIndex >= 0;
                ImU32 selCol = lit ? packColor(1.0f, 1.0f, 1.0f, 0.51f)
                                            : packColor(1.0f, 0.0f, 0.0f, 0.51f);
                drawDemodEdges(f, s, s.demods[(size_t)selectedIdx], viewW, viewH, selCol, s.hoverAlpha);
            }
            if (s.hoverIndex < 0) {
                float bw = selectedIdx >= 0 ? (float)s.demods[(size_t)selectedIdx].bw : 200000.0f;
                float ofs = bw / (float)s.spanHz;
                ImU32 selCol =
                    isNew ? packColor(0.0f, 1.0f, 0.0f, 0.51f)
                                        : packColor(1.0f, 1.0f, 0.0f, 0.51f);
                float cx = f.nx((mx01 - 0.5f) * 2.0f);
                f.dl->PushTexture(imguiAdditiveTexture());
                f.dl->AddLine(ImVec2(cx, f.ny(1.0f)), ImVec2(cx, f.ny(-1.0f)), selCol, 1.0f);
                if (std::strcmp(s.lastType, "USB") != 0) {
                    f.dl->AddLine(ImVec2(f.nx((mx01 - 0.5f) * 2.0f - ofs), f.ny(1.0f)),
                        ImVec2(f.nx((mx01 - 0.5f) * 2.0f - ofs), f.ny(-1.0f)), selCol, 1.0f);
                }
                if (std::strcmp(s.lastType, "LSB") != 0) {
                    f.dl->AddLine(ImVec2(f.nx((mx01 - 0.5f) * 2.0f + ofs), f.ny(1.0f)),
                        ImVec2(f.nx((mx01 - 0.5f) * 2.0f + ofs), f.ny(-1.0f)), selCol, 1.0f);
                }
                f.dl->PopTexture();
            } else if (!s.demods[(size_t)s.hoverIndex].selected) {
                drawDemodEdges(f, s, s.demods[(size_t)s.hoverIndex], viewW, viewH,
                    packColor(1.0f, 1.0f, 0.0f, 0.51f), s.hoverAlpha);
            }
        }
    } else {
        for (size_t i = 0; i < s.demods.size(); i++) {
            if (s.demods[i].selected) {
                drawDemodEdges(f, s, s.demods[(size_t)i], viewW, viewH,
                    packColor(1.0f, 1.0f, 1.0f, 0.51f), 0.0f);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void drawControl(AppState &s, int ww) {
    ImGui::SetNextWindowPos(ImVec2((float)ww - 340.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Waterfall spike");
    ImGui::Text("center %lld  span %lld", s.centerFreq, s.spanHz);
    ImGui::Text("lps %d  buffered %d  theme %s", s.lps, s.ring.bufferedLines(), s.theme == 0 ? "turbo" : "classic");
    ImGui::Text("hover %d  zone %d", s.hoverIndex, s.nextZone);
    ImGui::Text("snap %d  lastType %s", s.snap, s.lastType);
    ImGui::TextWrapped(
        "Wheel to zoom. Hover demods for edge highlights; drag centers and edges to move and resize. "
        "Click empty spectrum to move the selection. ALT-drag draws a range. "
        "T toggles theme, +/- lines per second, S snap, Y last type.");
    ImGui::End();
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    std::printf("waterfall via imgui draw lists; close the window to quit\n");
    std::fflush(stdout);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    const int startW = 1000;
    const int startH = 620;
    SDL_Window *window = SDL_CreateWindow("CubicSDR waterfall spike", startW, startH, SDL_WINDOW_RESIZABLE);
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
    buildLut(s);
    s.ring.setup(2048, 512);
    s.pacer.setLinesPerSecond(s.lps);
    for (int t = 0; t < 2; t++) {
        s.tex[t] = bgfx::createTexture2D(1024, 512, false, 1, bgfx::TextureFormat::RGBA8, 0, NULL);
        s.shadow[t].assign(1024 * 512, 0);
    }
    reexpandAll(s, 1024, 512);
    s.ring.setReady(true);

    SDL_Cursor *curCross = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    SDL_Cursor *curWE = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    SDL_Cursor *curAll = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    SDL_Cursor *curNow = nullptr;

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
                    if (e.key.scancode == SDL_SCANCODE_T) {
                        edges.tPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_EQUALS) {
                        edges.plusPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_MINUS) {
                        edges.minusPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_S) {
                        edges.sPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_Y) {
                        edges.yPressed = true;
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

        SDL_Keymod mods = SDL_GetModState();
        if (!edges.mouseLeft) {
            s.shiftDown = (mods & SDL_KMOD_SHIFT) != 0;
            s.altDown = (mods & SDL_KMOD_ALT) != 0;
        }

        uint64_t nowNs = SDL_GetTicksNS();
        float dt = (float)((nowNs - lastNs) / 1000000000.0);
        lastNs = nowNs;
        if (dt > 0.1f) {
            dt = 0.1f;
        }
        simT += dt;
        feedWaterfall(s, (double)dt, simT);
        int ww = startW;
        int wh = startH;
        SDL_GetWindowSize(window, &ww, &wh);

        imguiBeginFrame(in, ww, wh, dt);
        in.wheelX = 0.0f;
        in.wheelY = 0.0f;
        drawWaterfall(s, edges, mouseX, mouseY, ww, wh);
        drawControl(s, ww);
        imguiEndFrame(1);

        SDL_Cursor *want = curCross;
        if (s.nextZone == 1) {
            want = curAll;
        } else if (s.nextZone == 2 || s.nextZone == 3) {
            want = curWE;
        }
        if (want != curNow) {
            curNow = want;
            SDL_SetCursor(curNow);
        }

        bgfx::frame();
    }

    bgfx::destroy(s.tex[0]);
    bgfx::destroy(s.tex[1]);
    imguiShutdownBackend();
    bgfx::frame();
    bgfx::shutdown();
    SDL_DestroyCursor(curCross);
    SDL_DestroyCursor(curWE);
    SDL_DestroyCursor(curAll);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
