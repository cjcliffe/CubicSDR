// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <imgui.h>

#include <glm/glm.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>

#include "Utf8.h"
#include "imgui_bgfx.h"

struct ThemeColor {
    float r, g, b;
    float a = 1.0f;
};

inline uint8_t clampByte(float v) {
    if (v < 0.0f) {
        v = 0.0f;
    }
    if (v > 1.0f) {
        v = 1.0f;
    }
    return (uint8_t)(v * 255.0f);
}

inline ImU32 packColor(float r, float g, float b, float a = 1.0f) {
    return IM_COL32(clampByte(r), clampByte(g), clampByte(b), clampByte(a));
}

inline uint32_t packClear(const ThemeColor &bg) {
    return ((uint32_t)clampByte(bg.r) << 24) | ((uint32_t)clampByte(bg.g) << 16)
        | ((uint32_t)clampByte(bg.b) << 8) | 0xFFu;
}

enum class TextAlign { Left, Center, Right };

inline void drawTrackedText(ImDrawList *dl, const char *text, float xPx, float cyPx, float fontPx, ImU32 col,
    TextAlign align) {
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
    if (align == TextAlign::Right) {
        x = xPx - adv * (float)count;
    } else if (align == TextAlign::Center) {
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

inline void drawCenterText(
    ImDrawList *dl, const char *text, float cxPx, float cyPx, float fontPx, ImU32 col) {
    drawTrackedText(dl, text, cxPx, cyPx, fontPx, col, TextAlign::Center);
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
    float px(float x01) const {
        return ox + x01 * w;
    }
    float py(float y01) const {
        return ny(-0.75f + y01 * 1.5f);
    }
};

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

inline void drawPanelText(ImDrawList *dl, const Proj &f, const char *text, float ax, float ay, float fontPx,
    ImU32 col, TextAlign align, float vpW, float vpH) {
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
    if (align == TextAlign::Right) {
        penX = -advPx * (float)count;
    } else if (align == TextAlign::Center) {
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

inline void drawBarBgColumn(ImDrawList *dl, const Proj &f, float cx, const ThemeColor &c0,
    const ThemeColor &c1, float yLo, float yHi, float alpha) {
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

inline float plotX(float x01) {
    return x01 * 2.0f - 1.0f;
}

inline float plotY(float y01) {
    return -0.75f + y01 * 1.5f;
}

inline float labelFontSize(float bandPx) {
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

inline void centerText(const SpecFrame &f, const char *text, float cxPx, float cyPx, float fontPx) {
    ImGui::PushFont(imguiMonoFont(), fontPx);
    ImVec2 sz = ImGui::CalcTextSize(text);
    f.dl->AddText(ImVec2(cxPx - sz.x * 0.5f, cyPx - sz.y * 0.5f), IM_COL32_WHITE, text);
    ImGui::PopFont();
}

inline void emitHBar(const SpecFrame &f, float x0, float x1, float yTopPx, float yBotPx, ImU32 edgeL,
    ImU32 mid, ImU32 edgeR) {
    float xc = (x0 + x1) * 0.5f;
    f.dl->AddRectFilledMultiColor(ImVec2(x0, yTopPx), ImVec2(xc, yBotPx), edgeL, mid, mid, edgeL);
    f.dl->AddRectFilledMultiColor(ImVec2(xc, yTopPx), ImVec2(x1, yBotPx), mid, edgeR, edgeR, mid);
}

inline void emitVGradient(const SpecFrame &f, float ndx0, float ndx1, ImU32 edge, ImU32 full, bool drawTop,
    bool drawBottom) {
    float x0 = f.nx(ndx0);
    float x1 = f.nx(ndx1);
    if (drawTop) {
        f.dl->AddRectFilledMultiColor(ImVec2(x0, f.ny(1.0f)), ImVec2(x1, f.ny(0.0f)), edge, edge, full, full);
    }
    if (drawBottom) {
        f.dl->AddRectFilledMultiColor(ImVec2(x0, f.ny(0.0f)), ImVec2(x1, f.ny(-1.0f)), full, full, edge, edge);
    }
}
