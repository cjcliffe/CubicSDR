// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "imgui_bgfx.h"

#include <imgui.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <DemodMarkers.h>
#include <ScopeCarousel.h>
#include <SpectrumTicks.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

const float FRUSTUM_HALF = 0.82842712f;
const float EYE_Z = -1.205f;
const float PANEL_SPACING = 0.4f;

struct ThemeColor {
    float r, g, b, a;
};

struct SpikeTheme {
    ThemeColor scopeBackground;
    ThemeColor scopeLine;
    ThemeColor fftBackground;
    ThemeColor fftFill;
    ThemeColor fftLine;
    ThemeColor freqLine;
    ThemeColor text;
};

const SpikeTheme kTheme = {
    {0.1f, 0.1f, 0.1f, 1.0f},
    {0.9f, 0.9f, 0.9f, 1.0f},
    {0.1f, 0.1f, 0.1f, 1.0f},
    {0.15f, 0.35f, 0.85f, 0.3f},
    {0.9f, 0.9f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
};

struct AppState {
    int scopeMode = 0;
    bool showDb = true;
    bool leftDown = false;
    float lastX01 = 0.0f;
    float floorValue = 8.0f;
    float ceilValue = 400.0f;
    int fftSize = 1024;
    long long centerFreq = 12000;
    long long spanHz = 24000;
    std::vector<float> wave;
    std::vector<float> points;
    std::vector<float> peak;
    ScopeCarousel::CarouselState carousel;
};

struct FrameEdges {
    bool leftPressed = false;
    bool leftReleased = false;
    bool mouseLeft = false;
    bool bPressed = false;
    bool mPressed = false;
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

uint32_t packClear(const ThemeColor &bg) {
    return ((uint32_t)clampByte(bg.r) << 24) | ((uint32_t)clampByte(bg.g) << 16)
        | ((uint32_t)clampByte(bg.b) << 8) | 0xFFu;
}

struct Proj {
    glm::mat4 mvp;
    glm::mat4 invMvp;
    float ox, oy, w, h;
    bool project(float lx, float ly, float &sx, float &sy) const {
        glm::vec4 c = mvp * glm::vec4(lx, ly, 0.0f, 1.0f);
        if (c.w <= 0.0f) {
            return false;
        }
        float nx = c.x / c.w;
        float ny = c.y / c.w;
        sx = ox + (nx + 1.0f) * 0.5f * w;
        sy = oy + (1.0f - ny) * 0.5f * h;
        return true;
    }
    bool unprojectX(float cx, float &lx) const {
        float nx = (cx - ox) / w * 2.0f - 1.0f;
        glm::vec4 a = invMvp * glm::vec4(nx, 0.0f, -1.0f, 1.0f);
        glm::vec4 b = invMvp * glm::vec4(nx, 0.0f, 1.0f, 1.0f);
        if (a.w == 0.0f || b.w == 0.0f) {
            return false;
        }
        a /= a.w;
        b /= b.w;
        float dz = b.z - a.z;
        if (fabsf(dz) < 1e-6f) {
            return false;
        }
        float t = -a.z / dz;
        lx = a.x + t * (b.x - a.x);
        return true;
    }
};

Proj makePanelProj(float posX, float ox, float oy, float w, float h) {
    glm::mat4 proj = glm::frustum(-FRUSTUM_HALF, FRUSTUM_HALF, -FRUSTUM_HALF, FRUSTUM_HALF, 1.0f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, EYE_Z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    float tilt = ScopeCarousel::panelTiltDeg(posX);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(posX, 0.0f, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f))
        * glm::rotate(glm::mat4(1.0f), glm::radians(-tilt), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 mvp = proj * view * model;
    Proj p;
    p.mvp = mvp;
    p.invMvp = glm::inverse(mvp);
    p.ox = ox;
    p.oy = oy;
    p.w = w;
    p.h = h;
    return p;
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

unsigned int utf8Decode(const char *p, const char *end, int &bytes) {
    bytes = utf8CharBytes(p, end);
    unsigned int c = (unsigned char)p[0];
    if (bytes == 1) {
        return c;
    }
    if (bytes == 2) {
        return ((c & 0x1F) << 6) | ((unsigned char)p[1] & 0x3F);
    }
    if (bytes == 3) {
        return ((c & 0x0F) << 12) | (((unsigned char)p[1] & 0x3F) << 6) | ((unsigned char)p[2] & 0x3F);
    }
    return ((c & 0x07) << 18) | (((unsigned char)p[1] & 0x3F) << 12) | (((unsigned char)p[2] & 0x3F) << 6)
        | ((unsigned char)p[3] & 0x3F);
}

void drawPanelText(ImDrawList *dl, const Proj &f, const char *text, float ax, float ay, float fontPx,
    ImU32 col, int align, float vpW, float vpH) {
    ImFont *font = imguiMonoFont();
    ImFontBaked *baked = font->GetFontBaked(fontPx);
    const ImFontGlyph *ref = baked->FindGlyph('0');
    float refW = font->CalcTextSizeA(fontPx, FLT_MAX, 0.0f, "0", "0" + 1).x;
    float sc = (ref && ref->AdvanceX > 0.0f) ? refW / ref->AdvanceX : fontPx / 32.0f;
    float advPx = fontPx * 0.5f;
    const char *end = text + std::strlen(text);
    int count = 0;
    for (const char *p = text; p < end;) {
        int bytes = utf8CharBytes(p, end);
        count++;
        p += bytes;
    }
    float penX = 0.0f;
    if (align > 0) {
        penX = -advPx * (float)count;
    } else if (align == 0) {
        penX = -advPx * (float)count * 0.5f;
    }
    float penTop = -fontPx * 0.5f;
    ImTextureRef texRef = ImGui::GetIO().Fonts->TexRef;
    for (const char *p = text; p < end;) {
        int bytes = 0;
        unsigned int c = utf8Decode(p, end, bytes);
        const ImFontGlyph *g = baked->FindGlyph((ImWchar)c);
        if (g && g->Visible) {
            float qx0 = penX + g->X0 * sc;
            float qy0 = penTop + g->Y0 * sc;
            float qx1 = penX + g->X1 * sc;
            float qy1 = penTop + g->Y1 * sc;
            float p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y;
            if (f.project(ax + qx0 * 2.0f / vpW, ay - qy0 * 2.0f / vpH, p0x, p0y)
                && f.project(ax + qx1 * 2.0f / vpW, ay - qy0 * 2.0f / vpH, p1x, p1y)
                && f.project(ax + qx1 * 2.0f / vpW, ay - qy1 * 2.0f / vpH, p2x, p2y)
                && f.project(ax + qx0 * 2.0f / vpW, ay - qy1 * 2.0f / vpH, p3x, p3y)) {
                dl->AddImageQuad(texRef, ImVec2(p0x, p0y), ImVec2(p1x, p1y), ImVec2(p2x, p2y),
                    ImVec2(p3x, p3y), ImVec2(g->U0, g->V0), ImVec2(g->U1, g->V0), ImVec2(g->U1, g->V1),
                    ImVec2(g->U0, g->V1), col);
            }
        }
        penX += advPx;
        p += bytes;
    }
}

void drawTrackedText(ImDrawList *dl, const char *text, float xPx, float cyPx, float fontPx, ImU32 col,
    int align) {
    ImGui::PushFont(imguiMonoFont(), fontPx);
    float adv = fontPx * 0.5f;
    const char *end = text + std::strlen(text);
    int count = 0;
    for (const char *p = text; p < end;) {
        int bytes = utf8CharBytes(p, end);
        count++;
        p += bytes;
    }
    float x = xPx;
    if (align > 0) {
        x = xPx - adv * (float)count;
    } else if (align == 0) {
        x = xPx - adv * (float)count * 0.5f;
    }
    float y = cyPx - fontPx * 0.5f;
    for (const char *p = text; p < end;) {
        int bytes = utf8CharBytes(p, end);
        dl->AddText(ImVec2(x, y), col, p, p + bytes);
        x += adv;
        p += bytes;
    }
    ImGui::PopFont();
}

void synthWave(AppState &s, float t) {
    const int n = 512;
    s.wave.resize((size_t)n * 2);
    if (s.scopeMode == 2) {
        for (int i = 0; i < n; i++) {
            float u = (float)i / (float)n * 6.2831853f;
            s.wave[i * 2] = sinf(u * 3.0f + t) * 0.8f;
            s.wave[i * 2 + 1] = sinf(u * 2.0f) * 0.8f;
        }
        return;
    }
    if (s.scopeMode == 1) {
        for (int i = 0; i < n; i++) {
            float x = (float)(i % (n / 2)) / (float)(n / 2) * 2.0f - 1.0f;
            float ph = (float)i / (float)n * 6.2831853f * 3.0f + t * 4.0f;
            s.wave[i * 2] = x;
            s.wave[i * 2 + 1] = (i < n / 2 ? sinf(ph) : sinf(ph * 1.5f + 1.0f)) * 0.7f;
        }
        return;
    }
    for (int i = 0; i < n; i++) {
        float x = (float)i / (float)n * 2.0f - 1.0f;
        float ph = (float)i / (float)n * 6.2831853f * 3.0f + t * 4.0f;
        s.wave[i * 2] = x;
        s.wave[i * 2 + 1] = (sinf(ph) * 0.6f + sinf(ph * 3.0f) * 0.2f) * 0.8f;
    }
}

void synthSpectrum(AppState &s, float t) {
    const int n = 512;
    if (s.points.size() != (size_t)n * 2) {
        s.points.resize(n * 2);
        s.peak.assign(n * 2, 0.0f);
    }
    for (int i = 0; i < n; i++) {
        float x = (float)i / (float)n;
        float v = 0.08f + 0.04f * sinf(x * 71.0f + t * 2.0f);
        float dx0 = (x - 0.35f) / 0.02f;
        float dx1 = (x - 0.62f) / 0.008f;
        v += 0.6f * expf(-dx0 * dx0) + 0.8f * expf(-dx1 * dx1);
        s.points[i * 2] = x;
        s.points[i * 2 + 1] = v;
        float pk = s.peak[i * 2 + 1];
        pk = v > pk ? v : pk - 0.002f;
        if (pk < 0.0f) {
            pk = 0.0f;
        }
        s.peak[i * 2] = x;
        s.peak[i * 2 + 1] = pk;
    }
}

void drawBarBgColumn(ImDrawList *dl, const Proj &f, float cx, const ThemeColor &c0, const ThemeColor &c1,
    float yLo, float yHi, float alpha) {
    float lx;
    if (!f.unprojectX(cx + 0.5f, lx) || lx < -1.0f || lx > 1.0f) {
        return;
    }
    float x0, yBot, x1, yMid, x2, yTop;
    if (!f.project(lx, yLo, x0, yBot) || !f.project(lx, (yLo + yHi) * 0.5f, x1, yMid)
        || !f.project(lx, yHi, x2, yTop)) {
        return;
    }
    ImU32 edge = packColor(c0.r, c0.g, c0.b, alpha);
    ImU32 mid = packColor(c1.r, c1.g, c1.b, alpha);
    float top = yBot < yTop ? yBot : yTop;
    float bottom = yBot < yTop ? yTop : yBot;
    if (yMid < top) {
        yMid = top;
    }
    if (yMid > bottom) {
        yMid = bottom;
    }
    dl->AddRectFilledMultiColor(
        ImVec2(f.ox + cx, yMid), ImVec2(f.ox + cx + 1.0f, bottom), mid, mid, edge, edge);
    dl->AddRectFilledMultiColor(ImVec2(f.ox + cx, top), ImVec2(f.ox + cx + 1.0f, yMid), edge, edge, mid, mid);
}

void drawScopePanel(AppState &s, ImDrawList *dl, const Proj &f) {
    const ThemeColor &bg = kTheme.scopeBackground;
    const ThemeColor &line = kTheme.scopeLine;
    float bgAlpha = s.scopeMode == 2 ? 0.05f : 1.0f;
    if (s.scopeMode == 1) {
        for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
            drawBarBgColumn(dl, f, cx, bg, {bg.r * 2.0f, bg.g * 2.0f, bg.b * 2.0f, 1.0f}, 0.0f, 1.0f, bgAlpha);
            drawBarBgColumn(dl, f, cx, bg, {bg.r * 2.0f, bg.g * 2.0f, bg.b * 2.0f, 1.0f}, -1.0f, 0.0f, bgAlpha);
        }
    } else {
        for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
            drawBarBgColumn(dl, f, cx, bg, {bg.r * 2.0f, bg.g * 2.0f, bg.b * 2.0f, 1.0f}, -1.0f, 1.0f, bgAlpha);
        }
    }

    ImU32 axisDim = packColor(line.r * 0.35f, line.g * 0.35f, line.b * 0.35f);
    ImU32 axisFull = packColor(line.r, line.g, line.b);
    auto axisLine = [&](float y, ImU32 col) {
        float x0, y0, x1, y1;
        if (f.project(-1.0f, y, x0, y0) && f.project(1.0f, y, x1, y1)) {
            dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), col, 1.0f);
        }
    };
    if (s.scopeMode == 0) {
        axisLine(0.0f, axisDim);
    } else if (s.scopeMode == 1) {
        axisLine(0.0f, axisFull);
        axisLine(0.5f, axisDim);
        axisLine(-0.5f, axisDim);
    }

    ImU32 traceCol = packColor(line.r, line.g, line.b);
    size_t nPts = s.wave.size() / 2;
    if (s.scopeMode == 2) {
        dl->PushTexture(imguiAdditiveTexture());
        for (size_t i = 0; i < nPts; i++) {
            float sx, sy;
            if (f.project(s.wave[i * 2], s.wave[i * 2 + 1], sx, sy)) {
                dl->AddRectFilled(ImVec2(sx, sy), ImVec2(sx + 1.0f, sy + 1.0f), traceCol);
            }
        }
        dl->PopTexture();
    } else if (s.scopeMode == 1) {
        for (size_t i = 0; i + 1 < nPts / 2; i++) {
            float x0, y0, x1, y1;
            if (f.project(s.wave[i * 2], s.wave[i * 2 + 1] * 0.5f + 0.5f, x0, y0)
                && f.project(s.wave[i * 2 + 2], s.wave[i * 2 + 3] * 0.5f + 0.5f, x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), traceCol, 1.5f);
            }
        }
        for (size_t i = nPts / 2; i + 1 < nPts; i++) {
            float x0, y0, x1, y1;
            if (f.project(s.wave[i * 2], s.wave[i * 2 + 1] * 0.5f - 0.5f, x0, y0)
                && f.project(s.wave[i * 2 + 2], s.wave[i * 2 + 3] * 0.5f - 0.5f, x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), traceCol, 1.5f);
            }
        }
    } else {
        for (size_t i = 0; i + 1 < nPts; i++) {
            float x0, y0, x1, y1;
            if (f.project(s.wave[i * 2], s.wave[i * 2 + 1], x0, y0)
                && f.project(s.wave[i * 2 + 2], s.wave[i * 2 + 3], x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), traceCol, 1.5f);
            }
        }
    }

    drawPanelText(dl, f, "Test Device ", 1.0f, (f.h - 20.0f) / f.h, 12.0f,
        packColor(0.65f, 0.65f, 0.65f), 1, f.w, f.h);
}

float plotX(float x01) {
    return x01 * 2.0f - 1.0f;
}

float plotY(float y01) {
    return -0.75f + y01 * 1.5f;
}

void drawSpectrumPanel(AppState &s, ImDrawList *dl, const Proj &f) {
    const ThemeColor &bg = kTheme.scopeBackground;
    const ThemeColor &fill = kTheme.fftFill;
    const ThemeColor &line = kTheme.fftLine;
    const ThemeColor &freqLine = kTheme.freqLine;
    const ThemeColor &text = kTheme.text;

    ImU32 bgTop = packColor(0.0f, 0.0f, 0.0f);
    ImU32 bgBottom = packColor(bg.r * 2.0f, bg.g * 2.0f, bg.b * 2.0f);
    for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
        float lx;
        if (!f.unprojectX(f.ox + cx + 0.5f, lx) || lx < -1.0f || lx > 1.0f) {
            continue;
        }
        float x0, yTop, x1, yBottom;
        if (!f.project(lx, 1.0f, x0, yTop) || !f.project(lx, -1.0f, x1, yBottom)) {
            continue;
        }
        float top = yTop < yBottom ? yTop : yBottom;
        float bottom = yTop < yBottom ? yBottom : yTop;
        dl->AddRectFilledMultiColor(
            ImVec2(f.ox + cx, top), ImVec2(f.ox + cx + 1.0f, bottom), bgTop, bgTop, bgBottom, bgBottom);
    }

    if (!s.points.empty()) {
        SpectrumTicks::GridGeometry grid = SpectrumTicks::gridGeometry(s.floorValue, s.ceilValue);
        for (auto &band : grid.bands) {
            for (size_t i = 0; i + 3 < band.verts.size(); i += 4) {
                float ndcY = -0.75f + band.verts[i + 1] * 1.5f;
                float t = (1.0f - ndcY) * 0.5f;
                float lift = 0.12f * (float)band.alpha;
                ImU32 gridCol = packColor(bg.r * 2.0f * t + lift, bg.g * 2.0f * t + lift,
                    bg.b * 2.0f * t + lift);
                float x0, y0, x1, y1;
                if (f.project(plotX(band.verts[i]), plotY(band.verts[i + 1]), x0, y0)
                    && f.project(plotX(band.verts[i + 2]), plotY(band.verts[i + 3]), x1, y1)) {
                    dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), gridCol, 1.0f);
                }
            }
        }

        ImU32 fillCol = packColor(fill.r, fill.g, fill.b, fill.a);
        size_t nPts = s.points.size() / 2;
        size_t seg = 0;
        for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
            float lx;
            if (!f.unprojectX(f.ox + cx + 0.5f, lx) || lx < -1.0f || lx > 1.0f) {
                continue;
            }
            float x01 = (lx + 1.0f) * 0.5f;
            if (x01 < s.points[seg * 2]) {
                seg = 0;
            }
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
            float px0, topY, px1, bottomY;
            if (!f.project(lx, plotY(y), px0, topY) || !f.project(lx, plotY(0.0f), px1, bottomY)) {
                continue;
            }
            float top = topY < bottomY ? topY : bottomY;
            float bottom = topY < bottomY ? bottomY : topY;
            dl->AddRectFilled(ImVec2(f.ox + cx, top), ImVec2(f.ox + cx + 1.0f, bottom), fillCol);
        }

        ImU32 skirtTop = packColor(fill.r, fill.g, fill.b, fill.a);
        ImU32 skirtBottom = packColor(bg.r * 2.0f, bg.g * 2.0f, bg.b * 2.0f, fill.a);
        for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
            float lx;
            if (!f.unprojectX(f.ox + cx + 0.5f, lx) || lx < -1.0f || lx > 1.0f) {
                continue;
            }
            float px0, topY, px1, bottomY;
            if (!f.project(lx, -0.75f, px0, topY) || !f.project(lx, -1.0f, px1, bottomY)) {
                continue;
            }
            float top = topY < bottomY ? topY : bottomY;
            float bottom = topY < bottomY ? bottomY : topY;
            dl->AddRectFilledMultiColor(ImVec2(f.ox + cx, top), ImVec2(f.ox + cx + 1.0f, bottom), skirtTop,
                skirtTop, skirtBottom, skirtBottom);
        }

        ImU32 lineCol = packColor(line.r, line.g, line.b);
        for (size_t i = 0; i + 1 < nPts; i++) {
            float x0, y0, x1, y1;
            if (f.project(plotX(s.points[i * 2]), plotY(s.points[i * 2 + 1]), x0, y0)
                && f.project(plotX(s.points[i * 2 + 2]), plotY(s.points[i * 2 + 3]), x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), lineCol, 1.0f);
            }
        }
        size_t nHold = s.peak.size() / 2;
        for (size_t i = 0; i + 1 < nHold; i++) {
            float x0, y0, x1, y1;
            if (f.project(plotX(s.peak[i * 2]), plotY(s.peak[i * 2 + 1]), x0, y0)
                && f.project(plotX(s.peak[i * 2 + 2]), plotY(s.peak[i * 2 + 3]), x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(0, 255, 0, 128), 1.0f);
            }
        }
    }

    SpectrumTicks::AxisGeometry axis =
        SpectrumTicks::axisGeometry(s.centerFreq, s.spanHz, (double)f.w, (double)f.h, 1.0);
    ImU32 textCol = packColor(text.r, text.g, text.b);
    ImU32 majorCol = packColor(freqLine.r, freqLine.g, freqLine.b);
    ImU32 minorCol = packColor(freqLine.r * 0.65f, freqLine.g * 0.65f, freqLine.b * 0.65f);
    for (auto &tick : axis.ticks) {
        drawPanelText(dl, f, tick.label.c_str(), (float)tick.x, (float)axis.hPos, (float)axis.fontSize,
            textCol, 0, f.w, f.h);
    }
    for (auto &tick : axis.ticks) {
        float x0, y0, x1, y1;
        if (f.project((float)tick.x, (float)axis.tickBase, x0, y0)
            && f.project((float)tick.x, 1.0f, x1, y1)) {
            dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), tick.major ? majorCol : minorCol,
                tick.major ? 4.0f : 1.0f);
        }
    }

    if (s.showDb) {
        SpectrumTicks::DbLabels db =
            SpectrumTicks::dbLabels((double)s.ceilValue, (double)s.floorValue, s.fftSize, 0.0);
        float dbW = 88.0f / f.w;
        float dbH = 14.0f / f.h;
        for (int chip = 0; chip < 2; chip++) {
            float ly0 = chip == 0 ? 1.0f - 2.0f * dbH : -1.0f;
            float ly1 = chip == 0 ? 1.0f : -1.0f + 2.0f * dbH;
            for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
                float lx;
                if (!f.unprojectX(f.ox + cx + 0.5f, lx) || lx < -1.0f || lx > -1.0f + 2.0f * dbW) {
                    continue;
                }
                float u = (lx + 1.0f) / (2.0f * dbW);
                ImU32 col = IM_COL32(51, 51, 51, (int)(255.0f * (1.0f - u)));
                float px0, topY, px1, bottomY;
                if (!f.project(lx, ly1, px0, topY) || !f.project(lx, ly0, px1, bottomY)) {
                    continue;
                }
                float top = topY < bottomY ? topY : bottomY;
                float bottom = topY < bottomY ? bottomY : topY;
                dl->AddRectFilled(ImVec2(f.ox + cx, top), ImVec2(f.ox + cx + 1.0f, bottom), col);
            }
            drawPanelText(dl, f, (chip == 0 ? db.ceil : db.floor).c_str(), -1.0f + dbW,
                (ly0 + ly1) * 0.5f, 12.0f, IM_COL32_WHITE, 1, f.w, f.h);
        }
    }
}

void drawScope(AppState &s, const FrameEdges &edges, float mx, float my, int ww, int wh) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)ww, (float)wh), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Scope", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    bool hovered = !edges.mouseLeft && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    float mx01 = (mx - origin.x) / wsize.x;

    if (edges.mouseLeft) {
        s.leftDown = false;
    }
    if (edges.bPressed) {
        s.showDb = !s.showDb;
    }
    if (edges.mPressed) {
        s.scopeMode = (s.scopeMode + 1) % 3;
    }
    if (edges.leftPressed && hovered) {
        s.leftDown = true;
        s.lastX01 = mx01;
    }
    if (s.leftDown && hovered) {
        ScopeCarousel::dragStep(s.carousel, mx01 - s.lastX01);
        s.lastX01 = mx01;
    }
    if (edges.leftReleased) {
        s.leftDown = false;
    }

    float interval = ScopeCarousel::carouselInterval(PANEL_SPACING);
    ScopeCarousel::restStep(s.carousel, PANEL_SPACING, s.leftDown);
    float scopeX = s.carousel.ctr;
    bool showScope = ScopeCarousel::scopeVisibleStep(s.carousel, PANEL_SPACING);
    float specX = interval + s.carousel.ctr;
    bool showSpec = ScopeCarousel::spectrumVisibleStep(s.carousel, PANEL_SPACING);

    imguiSetClearColor(packClear(kTheme.scopeBackground));
    ImU32 bgTop = packColor(0.0f, 0.0f, 0.0f);
    ImU32 bgBottom =
        packColor(kTheme.scopeBackground.r * 3.0f, kTheme.scopeBackground.g * 3.0f, kTheme.scopeBackground.b * 3.0f);
    dl->AddRectFilledMultiColor(ImVec2(origin.x, origin.y + wsize.y * 0.5f),
        ImVec2(origin.x + wsize.x, origin.y + wsize.y), bgTop, bgTop, bgBottom, bgBottom);

    bool scopeFront = fabsf(scopeX) <= fabsf(specX);
    auto drawScopeF = [&]() {
        if (showScope) {
            Proj f = makePanelProj(scopeX, origin.x, origin.y, wsize.x, wsize.y);
            drawScopePanel(s, dl, f);
        }
    };
    auto drawSpecF = [&]() {
        if (showSpec) {
            Proj f = makePanelProj(specX, origin.x, origin.y, wsize.x, wsize.y);
            drawSpectrumPanel(s, dl, f);
        }
    };
    if (scopeFront) {
        drawSpecF();
        drawScopeF();
    } else {
        drawScopeF();
        drawSpecF();
    }

    float hPos = 13.0f / wsize.y;
    float titleY = origin.y + (1.0f - (-1.0f + hPos * 1.5f)) * 0.5f * wsize.y;
    ImU32 titleCol = packColor(0.65f, 0.65f, 0.65f);
    drawTrackedText(dl, "Frequency", origin.x + (-0.66f + 1.0f) * 0.5f * wsize.x, titleY, 12.0f, titleCol, 0);
    drawTrackedText(dl, "Bandwidth", origin.x + wsize.x * 0.5f, titleY, 12.0f, titleCol, 0);
    drawTrackedText(
        dl, "Center Frequency", origin.x + (0.66f + 1.0f) * 0.5f * wsize.x, titleY, 12.0f, titleCol, 0);
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void drawControl(AppState &s, int ww) {
    ImGui::SetNextWindowPos(ImVec2((float)ww - 340.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scope spike");
    const char *modes[] = {"Y", "2Y", "XY"};
    ImGui::Text("mode %s  dB %s  ctr %.2f", modes[s.scopeMode], s.showDb ? "on" : "off",
        (double)s.carousel.ctr);
    ImGui::TextWrapped("Drag to spin the carousel. M cycles scope mode, B toggles dB.");
    ImGui::End();
}

} // namespace

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    std::printf("scope via imgui draw lists; close the window to quit\n");
    std::fflush(stdout);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    const int startW = 900;
    const int startH = 450;
    SDL_Window *window = SDL_CreateWindow("CubicSDR scope spike", startW, startH, SDL_WINDOW_RESIZABLE);
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
                in.wheelX += e.wheel.x;
                in.wheelY += e.wheel.y;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!e.key.repeat) {
                    if (e.key.scancode == SDL_SCANCODE_B) {
                        edges.bPressed = true;
                    } else if (e.key.scancode == SDL_SCANCODE_M) {
                        edges.mPressed = true;
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
        synthWave(s, simT);
        synthSpectrum(s, simT);
        int ww = startW;
        int wh = startH;
        SDL_GetWindowSize(window, &ww, &wh);

        imguiBeginFrame(in, ww, wh, dt);
        in.wheelX = 0.0f;
        in.wheelY = 0.0f;
        drawScope(s, edges, mouseX, mouseY, ww, wh);
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
