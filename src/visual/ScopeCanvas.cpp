// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ScopeCanvas.h"

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ui/imgui/imgui_bgfx.h"
#include "DemodMarkers.h"
#include "SpectrumTicks.h"
#include "ColorTheme.h"
#include "GLFont.h"
#include "CubicSDR.h"
#include "AppFrame.h"

#include <cmath>
#include <cstring>

namespace {

const float FRUSTUM_HALF = 0.82842712f;
const float EYE_Z = -1.205f;

struct ThemeColor {
    float r, g, b;
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

float plotX(float x01) {
    return x01 * 2.0f - 1.0f;
}

float plotY(float y01) {
    return -0.75f + y01 * 1.5f;
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

void drawScopePanel(ImDrawList *dl, const Proj &f, int mode, const std::vector<float> &wave,
    const std::string &deviceName, const ThemeColor &bg, const ThemeColor &line, double fontScale,
    float viewHPx) {
    float bgAlpha = mode == SCOPE_MODE_XY ? 0.05f : 1.0f;
    ThemeColor bright{bg.r * 2.0f, bg.g * 2.0f, bg.b * 2.0f};
    if (mode == SCOPE_MODE_2Y) {
        for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
            drawBarBgColumn(dl, f, cx, bg, bright, 0.0f, 1.0f, bgAlpha);
            drawBarBgColumn(dl, f, cx, bg, bright, -1.0f, 0.0f, bgAlpha);
        }
    } else {
        for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
            drawBarBgColumn(dl, f, cx, bg, bright, -1.0f, 1.0f, bgAlpha);
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
    if (mode == SCOPE_MODE_Y) {
        axisLine(0.0f, axisDim);
    } else if (mode == SCOPE_MODE_2Y) {
        axisLine(0.0f, axisFull);
        axisLine(0.5f, axisDim);
        axisLine(-0.5f, axisDim);
    }

    ImU32 traceCol = packColor(line.r, line.g, line.b);
    size_t nPts = wave.size() / 2;
    if (mode == SCOPE_MODE_XY) {
        dl->PushTexture(imguiAdditiveTexture());
        for (size_t i = 0; i < nPts; i++) {
            float sx, sy;
            if (f.project(wave[i * 2], wave[i * 2 + 1], sx, sy)) {
                dl->AddRectFilled(ImVec2(sx, sy), ImVec2(sx + 1.0f, sy + 1.0f), traceCol);
            }
        }
        dl->PopTexture();
    } else if (mode == SCOPE_MODE_2Y) {
        for (size_t i = 0; i + 1 < nPts / 2; i++) {
            float x0, y0, x1, y1;
            if (f.project(wave[i * 2], wave[i * 2 + 1] * 0.5f + 0.5f, x0, y0)
                && f.project(wave[i * 2 + 2], wave[i * 2 + 3] * 0.5f + 0.5f, x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), traceCol, 1.5f);
            }
        }
        for (size_t i = nPts / 2; i + 1 < nPts; i++) {
            float x0, y0, x1, y1;
            if (f.project(wave[i * 2], wave[i * 2 + 1] * 0.5f - 0.5f, x0, y0)
                && f.project(wave[i * 2 + 2], wave[i * 2 + 3] * 0.5f - 0.5f, x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), traceCol, 1.5f);
            }
        }
    } else {
        for (size_t i = 0; i + 1 < nPts; i++) {
            float x0, y0, x1, y1;
            if (f.project(wave[i * 2], wave[i * 2 + 1], x0, y0)
                && f.project(wave[i * 2 + 2], wave[i * 2 + 3], x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), traceCol, 1.5f);
            }
        }
    }

    if (!deviceName.empty()) {
        drawPanelText(dl, f, deviceName.c_str(), 1.0f, (viewHPx - 20.0f) / viewHPx,
            12.0f * (float)fontScale, packColor(0.65f, 0.65f, 0.65f), 1, f.w, f.h);
    }
}

void drawSpectrumPanel(ImDrawList *dl, const Proj &f, const std::vector<float> &points, double floorValue,
    double ceilValue, int fftSize, long long centerFreq, long long spanHz, bool showDb,
    const ThemeColor &bg, const ThemeColor &fill, float fillAlpha, const ThemeColor &line,
    const ThemeColor &freqLine, const ThemeColor &text, double fontScale) {
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

    if (!points.empty()) {
        SpectrumTicks::GridGeometry grid = SpectrumTicks::gridGeometry((float)floorValue, (float)ceilValue);
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

        ImU32 fillCol = packColor(fill.r, fill.g, fill.b, fillAlpha);
        size_t nPts = points.size() / 2;
        size_t seg = 0;
        for (float cx = 0.0f; cx < f.w; cx += 1.0f) {
            float lx;
            if (!f.unprojectX(f.ox + cx + 0.5f, lx) || lx < -1.0f || lx > 1.0f) {
                continue;
            }
            float x01 = (lx + 1.0f) * 0.5f;
            if (x01 < points[seg * 2]) {
                seg = 0;
            }
            while (seg + 1 < nPts && points[(seg + 1) * 2] < x01) {
                seg++;
            }
            size_t s0 = seg * 2;
            size_t s1 = s0 + 2 < points.size() ? s0 + 2 : s0;
            float x0 = points[s0];
            float x1 = points[s1];
            float y = points[s0 + 1];
            if (x1 > x0) {
                float u = (x01 - x0) / (x1 - x0);
                if (u < 0.0f) {
                    u = 0.0f;
                }
                if (u > 1.0f) {
                    u = 1.0f;
                }
                y = points[s0 + 1] + (points[s1 + 1] - points[s0 + 1]) * u;
            }
            float px0, topY, px1, bottomY;
            if (!f.project(lx, plotY(y), px0, topY) || !f.project(lx, plotY(0.0f), px1, bottomY)) {
                continue;
            }
            float top = topY < bottomY ? topY : bottomY;
            float bottom = topY < bottomY ? bottomY : topY;
            dl->AddRectFilled(ImVec2(f.ox + cx, top), ImVec2(f.ox + cx + 1.0f, bottom), fillCol);
        }

        ImU32 skirtTop = packColor(fill.r, fill.g, fill.b, fillAlpha);
        ImU32 skirtBottom = packColor(bg.r * 2.0f, bg.g * 2.0f, bg.b * 2.0f, fillAlpha);
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
            if (f.project(plotX(points[i * 2]), plotY(points[i * 2 + 1]), x0, y0)
                && f.project(plotX(points[i * 2 + 2]), plotY(points[i * 2 + 3]), x1, y1)) {
                dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), lineCol, 1.0f);
            }
        }
    }

    SpectrumTicks::AxisGeometry axis =
        SpectrumTicks::axisGeometry(centerFreq, spanHz, (double)f.w, (double)f.h, fontScale);
    ImU32 textCol = packColor(text.r, text.g, text.b);
    ImU32 majorCol = packColor(freqLine.r, freqLine.g, freqLine.b);
    ImU32 minorCol = packColor(freqLine.r * 0.65f, freqLine.g * 0.65f, freqLine.b * 0.65f);
    float axisFontPx = (float)axis.fontSize * (float)fontScale;
    for (auto &tick : axis.ticks) {
        drawPanelText(dl, f, tick.label.c_str(), (float)tick.x, (float)axis.hPos, axisFontPx, textCol, 0,
            f.w, f.h);
    }
    for (auto &tick : axis.ticks) {
        float x0, y0, x1, y1;
        if (f.project((float)tick.x, (float)axis.tickBase, x0, y0)
            && f.project((float)tick.x, 1.0f, x1, y1)) {
            dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), tick.major ? majorCol : minorCol,
                tick.major ? 4.0f : 1.0f);
        }
    }

    if (showDb) {
        SpectrumTicks::DbLabels db = SpectrumTicks::dbLabels(ceilValue, floorValue, fftSize, 0.0);
        float dbW = 88.0f * (float)fontScale / f.w;
        float dbH = 14.0f * (float)fontScale / f.h;
        float chipFontPx = 12.0f * (float)fontScale;
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
                (ly0 + ly1) * 0.5f, chipFontPx, IM_COL32_WHITE, 1, f.w, f.h);
        }
    }
}

} // namespace

ScopeCanvas::ScopeCanvas(wxWindow *parent)
    : BgfxCanvas(parent), ppmMode(false), showDb(true), panelSpacing(0.4f), leftDown(false), lastX01(0.0f),
      scopeMode(SCOPE_MODE_Y), specFloor(0.0), specCeil(1.0), specFftSize(0), specCenter(0), specSpan(0) {
    inputData->set_max_num_items(2);
    SetCursor(wxCURSOR_SIZEWE);
}

ScopeCanvas::~ScopeCanvas() = default;

void ScopeCanvas::frameUpdate() {
    Refresh();
}

ScopeRenderDataQueuePtr ScopeCanvas::getInputQueue() {
    return inputData;
}

bool ScopeCanvas::scopeVisible() {
    return ScopeCarousel::scopeVisibleStep(carousel, panelSpacing);
}

bool ScopeCanvas::spectrumVisible() {
    return ScopeCarousel::spectrumVisibleStep(carousel, panelSpacing);
}

void ScopeCanvas::setDeviceName(std::string device_name) {
    deviceName = device_name;
    deviceName.append(" ");
}

void ScopeCanvas::setPPMMode(bool ppmMode_in) {
    ppmMode = ppmMode_in;
}

bool ScopeCanvas::getPPMMode() const {
    return ppmMode;
}

void ScopeCanvas::setShowDb(bool show) {
    showDb = show;
}

bool ScopeCanvas::getShowDb() const {
    return showDb;
}

void ScopeCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

void ScopeCanvas::onEnterView() {
    if (!helpTip.empty()) {
        wxGetApp().getAppFrame()->setStatusText(this, helpTip);
    }
    SetCursor(wxCURSOR_SIZEWE);
}

void ScopeCanvas::onLeaveView() {
}

void ScopeCanvas::OnKeyDown(wxKeyEvent& event) {
    switch (event.GetKeyCode()) {
    case 'B':
        setShowDb(!getShowDb());
        break;
    default:
        event.Skip();
    }
}

void ScopeCanvas::OnKeyUp(wxKeyEvent& event) {
    (void)event;
}

void ScopeCanvas::drawView(const BgfxEdges &edges, float mx, float my) {
    wxSize size = GetClientSize();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)size.x, (float)size.y), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Scope", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    bool hovered = isMouseInView() && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    float mx01 = (mx - origin.x) / wsize.x;

    ScopeRenderDataPtr avData;
    while (inputData->try_pop(avData)) {
        if (!avData) {
            continue;
        }
        if (!avData->spectrum) {
            scopeMode = (int)avData->mode;
            if (!avData->waveform_points.empty()) {
                wavePoints.assign(avData->waveform_points.begin(), avData->waveform_points.end());
            }
        } else {
            if (!avData->waveform_points.empty()) {
                specPoints.assign(avData->waveform_points.begin(), avData->waveform_points.end());
                specFloor = avData->fft_floor;
                specCeil = avData->fft_ceil;
                specSpan = (long long)(avData->sampleRate / 2) * 1000;
                specCenter = (long long)(avData->sampleRate / 4) * 1000;
                specFftSize = avData->fft_size;
            }
        }
    }

    if (edges.mouseLeft) {
        leftDown = false;
    }
    if (edges.leftPressed && hovered) {
        leftDown = true;
        lastX01 = mx01;
    }
    if (leftDown && hovered) {
        ScopeCarousel::dragStep(carousel, mx01 - lastX01);
        lastX01 = mx01;
    }
    if (edges.leftReleased) {
        leftDown = false;
    }

    float interval = ScopeCarousel::carouselInterval(panelSpacing);
    ScopeCarousel::restStep(carousel, panelSpacing, leftDown);
    float scopeX = carousel.ctr;
    bool showScope = ScopeCarousel::scopeVisibleStep(carousel, panelSpacing);
    float specX = interval + carousel.ctr;
    bool showSpec = ScopeCarousel::spectrumVisibleStep(carousel, panelSpacing);

    ColorTheme *theme = ThemeMgr::mgr.currentTheme;
    ThemeColor bg{theme->scopeBackground.r, theme->scopeBackground.g, theme->scopeBackground.b};
    ThemeColor line{theme->scopeLine.r, theme->scopeLine.g, theme->scopeLine.b};
    ThemeColor fill{theme->fftFill.r, theme->fftFill.g, theme->fftFill.b};
    ThemeColor fftLine{theme->fftLine.r, theme->fftLine.g, theme->fftLine.b};
    ThemeColor freqLine{theme->freqLine.r, theme->freqLine.g, theme->freqLine.b};
    ThemeColor text{theme->text.r, theme->text.g, theme->text.b};
    float fillAlpha = theme->fftFill.a;
    double fontScale = GLFont::getScaleFactor();
    imguiSetClearColor(packClear(bg));

    ImU32 bgTop = packColor(0.0f, 0.0f, 0.0f);
    ImU32 bgBottom = packColor(bg.r * 3.0f, bg.g * 3.0f, bg.b * 3.0f);
    dl->AddRectFilledMultiColor(ImVec2(origin.x, origin.y + wsize.y * 0.5f),
        ImVec2(origin.x + wsize.x, origin.y + wsize.y), bgTop, bgTop, bgBottom, bgBottom);

    bool scopeFront = fabsf(scopeX) <= fabsf(specX);
    float viewHPx = (float)GetClientSize().y * (float)GetContentScaleFactor();
    auto drawScopeF = [&]() {
        if (showScope) {
            drawScopePanel(dl, makePanelProj(scopeX, origin.x, origin.y, wsize.x, wsize.y), scopeMode,
                wavePoints, deviceName, bg, line, fontScale, viewHPx);
        }
    };
    auto drawSpecF = [&]() {
        if (showSpec) {
            drawSpectrumPanel(dl, makePanelProj(specX, origin.x, origin.y, wsize.x, wsize.y), specPoints,
                specFloor, specCeil, specFftSize, specCenter, specSpan, showDb, bg, fill, fillAlpha, fftLine,
                freqLine, text, fontScale);
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
    float titleY = origin.y + (1.0f - (-1.0f + hPos * (float)(fontScale + 0.5))) * 0.5f * wsize.y;
    ImU32 titleCol = packColor(0.65f, 0.65f, 0.65f);
    float titleFontPx = 12.0f * (float)fontScale;
    drawCenterText(dl, ppmMode ? "Device PPM" : "Frequency", origin.x + (-0.66f + 1.0f) * 0.5f * wsize.x,
        titleY, titleFontPx, titleCol);
    drawCenterText(dl, "Bandwidth", origin.x + wsize.x * 0.5f, titleY, titleFontPx, titleCol);
    drawCenterText(
        dl, "Center Frequency", origin.x + (0.66f + 1.0f) * 0.5f * wsize.x, titleY, titleFontPx, titleCol);
    ImGui::End();
    ImGui::PopStyleVar(2);
}
