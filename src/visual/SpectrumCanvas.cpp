// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "SpectrumCanvas.h"

#include <imgui.h>

#include "ui/imgui/imgui_bgfx.h"
#include "DemodMarkers.h"
#include "SpectrumTicks.h"
#include "WaterfallDrag.h"
#include "TuneRange.h"
#include "ColorTheme.h"
#include "FontScale.h"
#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include "AppFrame.h"
#include "Frequency.h"
#include "WaterfallCanvas.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstring>

namespace {

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

void drawTrackedText(const SpecFrame &f, const char *text, float xPx, float cyPx, float fontPx, ImU32 col,
    DemodMarkers::LabelAlign align) {
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
    if (align == DemodMarkers::LabelAlign::Right) {
        x = xPx - adv * (float)count;
    } else if (align == DemodMarkers::LabelAlign::Center) {
        x = xPx - adv * (float)count * 0.5f;
    }
    float y = cyPx - fontPx * 0.5f;
    for (const char *p = text; p < end;) {
        int bytes = utf8CharBytes(p, end);
        f.dl->AddText(ImVec2(x, y), col, p, p + bytes);
        x += adv;
        p += bytes;
    }
    ImGui::PopFont();
}

void drawCenterText(const SpecFrame &f, const char *text, float cxPx, float cyPx, float fontPx, ImU32 col) {
    drawTrackedText(f, text, cxPx, cyPx, fontPx, col, DemodMarkers::LabelAlign::Center);
}

void drawAlignText(const SpecFrame &f, const char *text, float xPx, float cyPx, float fontPx, ImU32 col,
    DemodMarkers::LabelAlign align) {
    drawTrackedText(f, text, xPx, cyPx, fontPx, col, align);
}

float dbChipFontSize(double fontScale) {
    float pdimy = (float)round(14.0 * fontScale);
    if (pdimy <= 16.0f) {
        return 12.0f;
    } else if (pdimy <= 18.0f) {
        return 16.0f;
    } else if (pdimy <= 24.0f) {
        return 18.0f;
    } else if (pdimy <= 32.0f) {
        return 24.0f;
    } else if (pdimy <= 48.0f) {
        return 32.0f;
    }
    return 48.0f;
}

} // namespace

SpectrumCanvas::SpectrumCanvas(wxWindow *parent, const std::string &windowName)
    : BgfxCanvas(parent), waterfallCanvas(nullptr), viewName(windowName), floorValue(0.0f), ceilValue(1.0f),
      fftSize(DEFAULT_FFT_SIZE), showDb(true), useDbOfs(true), isView(false), centerFreq(0), bandwidth(0),
      lastBandwidth(0), scaleFactor(1.0f), bwChange(0), resetScaleFactor(false), scaleFactorEnabled(false),
      shiftDown(false), altDown(false), ctrlDown(false), leftDown(false), rightDown(false), lastPanX01(0.0f),
      lastGainY01(0.0f), rightOriginY01(0.0f), mouseX01(0.0f), lastStatusX(-FLT_MAX),
      lastStatusY(-FLT_MAX), lastCursor(-1) {
    visualDataQueue->set_max_num_items(1);
    SetCursor(wxCURSOR_CROSS);
}

SpectrumCanvas::~SpectrumCanvas() = default;

void SpectrumCanvas::frameUpdate() {
    Refresh();
}

void SpectrumCanvas::attachWaterfallCanvas(WaterfallCanvas *canvas_in) {
    waterfallCanvas = canvas_in;
}

SpectrumVisualDataQueuePtr SpectrumCanvas::getVisualDataQueue() {
    return visualDataQueue;
}

float SpectrumCanvas::hoverX01() const {
    return mouseX01;
}

long long SpectrumCanvas::getFrequencyAt(float x) const {
    return WaterfallDrag::freqAtX(x, getCenterFrequency(), getBandwidth());
}

void SpectrumCanvas::setView(long long center_freq_in, long long bandwidth_in) {
    bwChange += (int)(bandwidth_in - bandwidth);
#define BW_RESET_TH 400000
    if (bwChange > BW_RESET_TH || bwChange < -BW_RESET_TH) {
        resetScaleFactor = true;
        bwChange = 0;
    }
    isView = true;
    centerFreq = center_freq_in;
    bandwidth = bandwidth_in;
    lastBandwidth = 0;
}

void SpectrumCanvas::disableView() {
    isView = false;
    centerFreq = wxGetApp().getFrequency();
    bandwidth = wxGetApp().getSampleRate();
    lastBandwidth = 0;
}

bool SpectrumCanvas::getViewState() const {
    return isView;
}

void SpectrumCanvas::setCenterFrequency(long long center_freq_in) {
    centerFreq = center_freq_in;
}

long long SpectrumCanvas::getCenterFrequency() const {
    if (isView) {
        return centerFreq;
    }
    return wxGetApp().getFrequency();
}

void SpectrumCanvas::setBandwidth(long long bandwidth_in) {
    bandwidth = bandwidth_in;
}

long long SpectrumCanvas::getBandwidth() const {
    if (isView) {
        return bandwidth;
    }
    return wxGetApp().getSampleRate();
}

void SpectrumCanvas::moveCenterFrequency(long long freqChange) {
    long long freq = wxGetApp().getFrequency();

    if (isView) {
        centerFreq -= freqChange;

        if (waterfallCanvas) {
            waterfallCanvas->setCenterFrequency(centerFreq);
        }

        long long bwOfs = (centerFreq > freq) ? ((long long)bandwidth / 2) : (-(long long)bandwidth / 2);
        long long freqEdge = centerFreq + bwOfs;

        if (abs(freq - freqEdge) > (wxGetApp().getSampleRate() / 2)) {
            freqChange = -((centerFreq > freq) ? (freqEdge - freq - (wxGetApp().getSampleRate() / 2))
                                              : (freqEdge - freq + (wxGetApp().getSampleRate() / 2)));
        } else {
            freqChange = 0;
        }
    }

    if (freqChange) {
        freq -= freqChange;
        wxGetApp().setFrequency(freq);
    }

    if (isView) {
        long long pinned =
            pinViewCenter(centerFreq, bandwidth, wxGetApp().getFrequency(), wxGetApp().getSampleRate());
        if (pinned != centerFreq) {
            centerFreq = pinned;
            if (waterfallCanvas) {
                waterfallCanvas->setCenterFrequency(centerFreq);
            }
        }
    }
}

void SpectrumCanvas::setShowDb(bool showDb_in) {
    showDb = showDb_in;
}

bool SpectrumCanvas::getShowDb() const {
    return showDb;
}

void SpectrumCanvas::setUseDBOfs(bool useDbOfs_in) {
    useDbOfs = useDbOfs_in;
}

bool SpectrumCanvas::getUseDBOfs() const {
    return useDbOfs;
}

void SpectrumCanvas::setScaleFactorEnabled(bool en) {
    scaleFactorEnabled = en;
}

void SpectrumCanvas::setFFTSize(int fftSize_in) {
    fftSize = fftSize_in;
}

void SpectrumCanvas::updateScaleFactor(float factor) {
    SpectrumVisualProcessor *sp = wxGetApp().getSpectrumProcessor();
    FFTVisualDataThread *wdt = wxGetApp().getAppFrame()->getWaterfallDataThread();
    SpectrumVisualProcessor *wp = wdt->getProcessor();

    scaleFactor = factor;
    sp->setScaleFactor(factor);
    wp->setScaleFactor(factor);
    if (wxGetApp().getDemodSpectrumProcessor()) {
        wxGetApp().getDemodSpectrumProcessor()->setScaleFactor(factor);
    }
}

void SpectrumCanvas::updateScaleFactorFromYMove(float yDeltaMouseMove) {
    scaleFactor += yDeltaMouseMove * 2.0f;

    if (scaleFactor < 0.25f) {
        scaleFactor = 0.25f;
    }
    if (scaleFactor > 10.0f) {
        scaleFactor = 10.0f;
    }

    resetScaleFactor = false;
    updateScaleFactor(scaleFactor);
}

void SpectrumCanvas::resetScaleFactorFromClick() {
    scaleFactor = wxGetApp().getSpectrumProcessor()->getScaleFactor();
    resetScaleFactor = true;

    wxGetApp().getSpectrumProcessor()->setPeakHold(wxGetApp().getSpectrumProcessor()->getPeakHold());

    if (wxGetApp().getDemodSpectrumProcessor()) {
        wxGetApp().getDemodSpectrumProcessor()->setPeakHold(wxGetApp().getSpectrumProcessor()->getPeakHold());
    }
}

void SpectrumCanvas::updateCursor(bool left, bool right) {
    wxStockCursor code = left ? wxCURSOR_SIZEWE : (right ? wxCURSOR_SIZENS : wxCURSOR_CROSS);
    if ((int)code != lastCursor) {
        lastCursor = (int)code;
        SetCursor(code);
    }
}

void SpectrumCanvas::onEnterView() {
    SetCursor(wxCURSOR_CROSS);
    lastCursor = (int)wxCURSOR_CROSS;
#ifdef _WIN32
    if (wxGetApp().getAppFrame()->canFocus()) {
        SetFocus();
    }
#endif
}

void SpectrumCanvas::onLeaveView() {
    SetCursor(wxCURSOR_CROSS);
    lastCursor = (int)wxCURSOR_CROSS;
    leftDown = false;
    rightDown = false;
}

void SpectrumCanvas::OnKeyDown(wxKeyEvent& event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    switch (event.GetKeyCode()) {
    case 'B':
        setShowDb(!getShowDb());
        break;
    default:
        event.Skip();
    }
}

void SpectrumCanvas::OnKeyUp(wxKeyEvent& event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void SpectrumCanvas::drawView(const BgfxEdges &edges, float mx, float my) {
    wxSize size = GetClientSize();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)size.x, (float)size.y), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin(viewName.c_str(), nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    SpecFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, wsize.x, wsize.y};
    bool hovered = isMouseInView() && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    float mx01 = (mx - origin.x) / wsize.x;
    float my01 = 1.0f - (my - origin.y) / wsize.y;
    if (hovered) {
        mouseX01 = mx01;
    }

    SpectrumVisualDataPtr vData;
    if (visualDataQueue->try_pop(vData)) {
        if (vData) {
            points.assign(vData->spectrum_points.begin(), vData->spectrum_points.end());
            peakPoints.assign(vData->spectrum_hold_points.begin(), vData->spectrum_hold_points.end());
            floorValue = (float)vData->fft_floor;
            ceilValue = (float)vData->fft_ceiling;
        }
    }

    if (resetScaleFactor) {
        scaleFactor += (1.0f - scaleFactor) * 0.05f;
        if (fabsf(scaleFactor - 1.0f) < 0.01f) {
            scaleFactor = 1.0f;
            resetScaleFactor = false;
        }
        updateScaleFactor(scaleFactor);
    }

    if (edges.mouseLeft) {
        leftDown = false;
        rightDown = false;
    }
    if (edges.leftPressed && hovered && !rightDown) {
        leftDown = true;
        lastPanX01 = mx01;
    }
    if (leftDown && hovered) {
        int freqChange = (int)((mx01 - lastPanX01) * (float)getBandwidth());
        if (freqChange != 0) {
            moveCenterFrequency(freqChange);
        }
        lastPanX01 = mx01;
    }
    if (edges.leftReleased) {
        leftDown = false;
    }
    if (edges.wheelRotV != 0 && hovered && waterfallCanvas) {
        waterfallCanvas->setMouseZoom(WaterfallDrag::wheelZoom(edges.wheelRotV, 120));
    }
    if (edges.rightPressed && hovered && !leftDown) {
        rightDown = true;
        lastGainY01 = my01;
        rightOriginY01 = my01;
        scaleFactor = wxGetApp().getSpectrumProcessor()->getScaleFactor();
    }
    if (scaleFactorEnabled && rightDown && hovered && !leftDown) {
        updateScaleFactorFromYMove(my01 - lastGainY01);
        lastGainY01 = my01;
    }
    if (edges.rightReleased) {
        if (rightDown && (my01 - rightOriginY01) == 0.0f) {
            resetScaleFactorFromClick();
        }
        rightDown = false;
    }
    updateCursor(leftDown, rightDown);

    bool dragging = leftDown || (scaleFactorEnabled && rightDown);
    if (hovered && !dragging && (mx != lastStatusX || my != lastStatusY)) {
        lastStatusX = mx;
        lastStatusY = my;
        if (scaleFactorEnabled) {
            wxGetApp().getAppFrame()->setStatusText(this,
                "Drag horizontal to adjust center frequency. Arrow keys or wheel to navigate/zoom bandwidth. "
                "Right-drag or SHIFT+UP/DOWN to adjust visual gain, right-click to reset it. 'B' to toggle decibels "
                "display.");
        } else {
            auto demodTip = wxGetApp().getDemodMgr().getCurrentModem();
            if (demodTip) {
                wxGetApp().getAppFrame()->setStatusText(this,
                    "Spectrum of " + demodTip->getDemodulatorType() + ": "
                        + frequencyToStr(demodTip->getFrequency()));
            } else {
                wxGetApp().getAppFrame()->setStatusText(this, "Displaying spectrum of active demodulator.");
            }
        }
    }

    ColorTheme *theme = ThemeMgr::mgr.currentTheme;
    ThemeColor bg{theme->fftBackground.r, theme->fftBackground.g, theme->fftBackground.b};
    ThemeColor fill{theme->fftFill.r, theme->fftFill.g, theme->fftFill.b};
    ThemeColor line{theme->fftLine.r, theme->fftLine.g, theme->fftLine.b};
    ThemeColor highlight{theme->fftHighlight.r, theme->fftHighlight.g, theme->fftHighlight.b};
    ThemeColor freqLine{theme->freqLine.r, theme->freqLine.g, theme->freqLine.b};
    ThemeColor text{theme->text.r, theme->text.g, theme->text.b};
    ThemeColor hover{theme->waterfallHover.r, theme->waterfallHover.g, theme->waterfallHover.b};
    ThemeColor isNew{theme->waterfallNew.r, theme->waterfallNew.g, theme->waterfallNew.b};
    float fillAlpha = theme->fftFill.a;
    imguiSetClearColor(packClear(bg));

    ImU32 bgTop = packColor(bg.r, bg.g, bg.b);
    ImU32 bgBottom = packColor(bg.r * 2.0f, bg.g * 2.0f, bg.b * 2.0f);
    f.dl->AddRectFilledMultiColor(ImVec2(f.nx(-1.0f), f.ny(1.0f)), ImVec2(f.nx(1.0f), f.ny(-1.0f)), bgTop, bgTop,
        bgBottom, bgBottom);

    if (!points.empty()) {
        SpectrumTicks::GridGeometry grid = SpectrumTicks::gridGeometry(floorValue, ceilValue);
        for (auto &band : grid.bands) {
            for (size_t i = 0; i + 3 < band.verts.size(); i += 4) {
                float ndcY = -0.75f + band.verts[i + 1] * 1.5f;
                float t = (1.0f - ndcY) * 0.5f;
                float lift = 0.12f * (float)band.alpha;
                ImU32 gridCol = packColor(bg.r * (1.0f + t) + lift, bg.g * (1.0f + t) + lift,
                    bg.b * (1.0f + t) + lift);
                f.dl->AddLine(ImVec2(f.px(band.verts[i]), f.py(band.verts[i + 1])),
                    ImVec2(f.px(band.verts[i + 2]), f.py(band.verts[i + 3])), gridCol, 1.0f);
            }
        }

        ImU32 fillCol = packColor(fill.r, fill.g, fill.b, fillAlpha);
        size_t nPts = points.size() / 2;
        float bottomY = f.py(0.0f);
        size_t seg = 0;
        for (float cx = 0.0f; cx < wsize.x; cx += 1.0f) {
            float x01 = (cx + 0.5f) / wsize.x;
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
            float topY = f.py(y);
            if (topY > bottomY) {
                topY = bottomY;
            }
            f.dl->AddRectFilled(ImVec2(f.ox + cx, topY), ImVec2(f.ox + cx + 1.0f, bottomY), fillCol);
        }
        ImU32 skirtTop = packColor(fill.r, fill.g, fill.b, fillAlpha);
        ImU32 skirtBottom = packColor(bg.r, bg.g, bg.b, fillAlpha);
        f.dl->AddRectFilledMultiColor(ImVec2(f.nx(-1.0f), f.ny(-0.75f)), ImVec2(f.nx(1.0f), f.ny(-1.0f)), skirtTop,
            skirtTop, skirtBottom, skirtBottom);

        ImU32 lineCol = packColor(line.r, line.g, line.b);
        for (size_t i = 0; i + 1 < nPts; i++) {
            f.dl->AddLine(ImVec2(f.px(points[i * 2]), f.py(points[i * 2 + 1])),
                ImVec2(f.px(points[i * 2 + 2]), f.py(points[i * 2 + 3])), lineCol, 1.0f);
        }
        if (!peakPoints.empty()) {
            size_t nHold = peakPoints.size() / 2;
            for (size_t i = 0; i + 1 < nHold; i++) {
                f.dl->AddLine(ImVec2(f.px(peakPoints[i * 2]), f.py(peakPoints[i * 2 + 1])),
                    ImVec2(f.px(peakPoints[i * 2 + 2]), f.py(peakPoints[i * 2 + 3])), IM_COL32(0, 255, 0, 128),
                    1.0f);
            }
        }
    }

    double fontScale = FontScale::getScaleFactor();
    SpectrumTicks::AxisGeometry axis = SpectrumTicks::axisGeometry(getCenterFrequency(), getBandwidth(),
        (double)wsize.x, (double)wsize.y, fontScale);
    ImU32 textCol = packColor(text.r, text.g, text.b);
    ImU32 majorCol = packColor(freqLine.r, freqLine.g, freqLine.b);
    ImU32 minorCol = packColor(freqLine.r * 0.65f, freqLine.g * 0.65f, freqLine.b * 0.65f);
    float axisFontPx = (float)axis.fontSize * (float)fontScale;
    for (auto &tick : axis.ticks) {
        drawCenterText(
            f, tick.label.c_str(), f.nx((float)tick.x), f.ny((float)axis.hPos), axisFontPx, textCol);
    }
    for (auto &tick : axis.ticks) {
        float tx = f.nx((float)tick.x);
        f.dl->AddLine(ImVec2(tx, f.ny((float)axis.tickBase)), ImVec2(tx, f.ny(1.0f)),
            tick.major ? majorCol : minorCol, tick.major ? 4.0f : 1.0f);
    }

    int viewW = (int)wsize.x;
    int viewH = (int)wsize.y;
    float labelFontPx = 16.0f * (float)fontScale;
    auto demods = wxGetApp().getDemodMgr().getDemodulators();
    auto selectedDemodulator = wxGetApp().getDemodMgr().getCurrentModem();
    for (auto &demod : demods) {
        if (!demod->isActive()) {
            continue;
        }
        bool isSelected = (selectedDemodulator == demod);
        const ThemeColor &color = isSelected ? hover : highlight;
        long long demodFreq = demod->getFrequency();
        if (demod->isDeltaLock()) {
            demodFreq = wxGetApp().getFrequency() + demod->getDeltaLockOfs();
        }
        bool soloMode = wxGetApp().getSoloMode();
        long long timeMs = 0;
        if (demod->isRecording()) {
            timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                         .count();
        }
        DemodMarkers::MarkerInput in;
        in.freqHz = demodFreq;
        in.bandwidthHz = demod->getBandwidth();
        in.sideband = DemodMarkers::sidebandForType(demod->getDemodulatorType());
        in.viewCenterHz = getCenterFrequency();
        in.viewSpanHz = getBandwidth();
        in.viewWidthPx = viewW;
        in.viewHeightPx = viewH;
        in.deltaLocked = demod->isDeltaLock();
        in.muted = demod->isMuted();
        in.solo = soloMode && isSelected;
        in.soloMode = soloMode;
        in.recording = demod->isRecording();
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
        if (isSelected) {
            float cx = f.nx(geo.uxPos);
            f.dl->AddLine(ImVec2(cx, f.ny(1.0f)), ImVec2(cx, f.ny(-1.0f)),
                packColor(color.r, color.g, color.b, 0.5f), 1.0f);
        }
        std::string demodLabel = geo.prefix;
        demodLabel.append(demod->getLabel());
        drawAlignText(f, demodLabel.c_str(), f.nx(geo.uxPos), f.ny(geo.hPos), labelFontPx,
            IM_COL32(255, 255, 255, 204), geo.align);
    }

    if (waterfallCanvas && !waterfallCanvas->getHoverDemod()) {
        if (waterfallCanvas->isMouseInView()) {
            int snap = wxGetApp().getFrequencySnap();
            long long freq = getFrequencyAt(waterfallCanvas->hoverX01());
            if (snap > 1) {
                freq = (long long)(roundf((float)freq / (float)snap) * (float)snap);
            }
            bool ghostNew = (((waterfallCanvas->isShiftHeld()
                                   || (selectedDemodulator && !selectedDemodulator->isActive()))
                                   && selectedDemodulator)
                || (!selectedDemodulator));
            const ThemeColor &color = ghostNew ? isNew : hover;
            DemodMarkers::MarkerInput in;
            in.freqHz = freq;
            in.bandwidthHz = wxGetApp().getDemodMgr().getLastBandwidth();
            in.sideband =
                DemodMarkers::sidebandForType(wxGetApp().getDemodMgr().getLastDemodulatorType());
            in.viewCenterHz = getCenterFrequency();
            in.viewSpanHz = getBandwidth();
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
            drawAlignText(
                f, geo.freqText.c_str(), lx + dxPx, ly + dyPx, labelFontPx, IM_COL32(0, 0, 0, 255), geo.align);
            drawAlignText(
                f, geo.freqText.c_str(), lx - dxPx, ly - dyPx, labelFontPx, IM_COL32(0, 0, 0, 255), geo.align);
            drawAlignText(f, geo.freqText.c_str(), lx, ly, labelFontPx, mainCol, geo.align);
        }
    }

    if (showDb) {
        float dbOfs = useDbOfs ? (float)wxGetApp().getConfig()->getDBOffset() : 0.0f;
        SpectrumTicks::DbLabels db =
            SpectrumTicks::dbLabels((double)ceilValue, (double)floorValue, fftSize, (double)dbOfs);
        ImU32 chipL = IM_COL32(51, 51, 51, 255);
        ImU32 chipR = IM_COL32(51, 51, 51, 0);
        float chipW = 88.0f * (float)fontScale;
        float chipH = 14.0f * (float)fontScale;
        float chipFontPx = dbChipFontSize(fontScale);
        f.dl->AddRectFilledMultiColor(ImVec2(f.ox, f.oy), ImVec2(f.ox + chipW, f.oy + chipH), chipL, chipR, chipR,
            chipL);
        drawAlignText(f, db.ceil.c_str(), f.ox + chipW * 0.5f, f.oy + chipH * 0.5f, chipFontPx, IM_COL32_WHITE,
            DemodMarkers::LabelAlign::Right);
        f.dl->AddRectFilledMultiColor(ImVec2(f.ox, f.oy + wsize.y - chipH), ImVec2(f.ox + chipW, f.oy + wsize.y),
            chipL, chipR, chipR, chipL);
        drawAlignText(f, db.floor.c_str(), f.ox + chipW * 0.5f, f.oy + wsize.y - chipH * 0.5f, chipFontPx,
            IM_COL32_WHITE, DemodMarkers::LabelAlign::Right);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}
