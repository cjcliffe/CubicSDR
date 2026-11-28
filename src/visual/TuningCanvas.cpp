// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "TuningCanvas.h"

#include <imgui.h>

#include "ui/imgui/imgui_bgfx.h"
#include "ui/imgui/imgui_draw_util.h"
#include "TunerStep.h"
#include "TunerLayout.h"
#include "TuneRange.h"
#include "ColorTheme.h"
#include "CubicSDR.h"
#include "AppFrame.h"
#include "DemodulatorMgr.h"
#include "FrequencyDialog.h"

#include <cmath>
#include <string>
#include <vector>

namespace {

const float kFreqDP = -1.0f;
const float kFreqW = (1.0f / 3.0f) * 2.0f;
const float kBwDP = -1.0f + (2.25f / 3.0f);
const float kBwW = (1.0f / 4.0f) * 2.0f;
const float kCenterDP = -1.0f + (2.0f / 3.0f) * 2.0f;
const float kCenterW = (1.0f / 3.0f) * 2.0f;

const int kFreqCount = 11;
const int kBwCount = 7;

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



using TunerFrame = SpecFrame;

void drawBarQuads(const TunerFrame &f, const TunerLayout::TunerBars &bars, const ThemeColor &color,
    const ThemeColor &bg) {
    ImU32 edge = packColor(color.r * 0.5f + bg.r, color.g * 0.5f + bg.g, color.b * 0.5f + bg.b);
    ImU32 full = packColor(color.r + bg.r, color.g + bg.g, color.b + bg.b);
    if (bars.top.empty() && bars.bottom.empty()) {
        return;
    }
    float x0 = !bars.top.empty() ? bars.top[2] : bars.bottom[2];
    float x1 = !bars.top.empty() ? bars.top[0] : bars.bottom[0];
    emitVGradient(f, x0, x1, edge, full, !bars.top.empty(), !bars.bottom.empty());
}

void drawBarQuadsStacked(const TunerFrame &f, const TunerLayout::TunerBars &bars, const ThemeColor &over,
    const ThemeColor &under, const ThemeColor &bg) {
    ImU32 edge = packColor(over.r * 0.5f + under.r * 0.5f + bg.r, over.g * 0.5f + under.g * 0.5f + bg.g,
        over.b * 0.5f + under.b * 0.5f + bg.b);
    ImU32 full = packColor(
        over.r + under.r + bg.r, over.g + under.g + bg.g, over.b + under.b + bg.b);
    if (bars.top.empty() && bars.bottom.empty()) {
        return;
    }
    float x0 = !bars.top.empty() ? bars.top[2] : bars.bottom[2];
    float x1 = !bars.top.empty() ? bars.top[0] : bars.bottom[0];
    emitVGradient(f, x0, x1, edge, full, !bars.top.empty(), !bars.bottom.empty());
}

ThemeColor regionColorAt(int index, int count, const ThemeColor &light, const ThemeColor &dark) {
    if (count == kBwCount) {
        return (index >= 4 && index <= 6) ? dark : light;
    }
    return ((index >= 4 && index <= 6) || index >= 10) ? dark : light;
}

void drawDragBar(const TunerFrame &f, float p1, float p2, const ThemeColor &drag, const ThemeColor &light,
    const ThemeColor &dark, const ThemeColor &bg) {
    std::vector<float> q = TunerLayout::dragBarQuads(p1, p2);
    float ndx0 = q[0] < q[2] ? q[0] : q[2];
    float ndx1 = q[0] < q[2] ? q[2] : q[0];
    ImU32 edge = packColor(drag.r * 0.5f + bg.r, drag.g * 0.5f + bg.g, drag.b * 0.5f + bg.b);
    ImU32 full = packColor(drag.r + bg.r, drag.g + bg.g, drag.b + bg.b);
    emitVGradient(f, ndx0, ndx1, edge, full, true, true);
    for (const BarGroup &g : kBarGroups) {
        TunerLayout::TunerBars bars = TunerLayout::tunerBarQuads(g.start, g.end, g.count, g.dp, g.w, true, true);
        float ox0 = ndx0 > bars.top[2] ? ndx0 : bars.top[2];
        float ox1 = ndx1 < bars.top[0] ? ndx1 : bars.top[0];
        if (ox1 > ox0) {
            const ThemeColor &under = g.light ? light : dark;
            ImU32 sEdge = packColor(drag.r * 0.5f + under.r * 0.5f + bg.r, drag.g * 0.5f + under.g * 0.5f + bg.g,
                drag.b * 0.5f + under.b * 0.5f + bg.b);
            ImU32 sFull = packColor(
                drag.r + under.r + bg.r, drag.g + under.g + bg.g, drag.b + under.b + bg.b);
            emitVGradient(f, ox0, ox1, sEdge, sFull, true, true);
        }
    }
}

void drawTunerDigits(const TunerFrame &f, long long value, int count, float dp, float w, float fontScale,
    ImU32 textCol) {
    std::string text = std::to_string(value);
    std::vector<TunerLayout::TunerChar> chars = TunerLayout::tunerChars(count, (int)text.size(), dp, w);
    ImGui::PushFont(imguiMonoFont(), 32.0f * fontScale);
    float midY = f.ny(0.0f);
    for (const TunerLayout::TunerChar &ch : chars) {
        char glyph[2] = {text[(size_t)ch.charIndex], '\0'};
        ImVec2 sz = ImGui::CalcTextSize(glyph);
        f.dl->AddText(ImVec2(f.nx(ch.x) - sz.x * 0.5f, midY - sz.y * 0.5f), textCol, glyph);
    }
    ImGui::PopFont();
}

void drawTunerGrid(const TunerFrame &f, int count, float dp, float w) {
    std::vector<float> verts = TunerLayout::tunerGridVerts(count, dp, w);
    ImU32 col = packColor(0.65f, 0.65f, 0.65f, 0.25f);
    for (size_t i = 0; i + 3 < verts.size(); i += 4) {
        f.dl->AddLineV(floorf(f.nx(verts[i])), f.ny(1.0f), f.ny(-1.0f), col);
    }
}

void drawSnapBox(const TunerFrame &f, int snapExp, float viewHeightPx) {
    std::vector<float> box = TunerLayout::tunerDigitBox(snapExp, kFreqCount, kFreqDP, kFreqW, viewHeightPx);
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

const char *hoverTip(TuningCanvas::ActiveState state) {
    switch (state) {
    case TuningCanvas::TUNING_HOVER_FREQ:
        return "Click, wheel or drag(left/right) a digit to change frequency; SPACE for direct input. Right click to set/clear snap. Hold ALT to change PPM. Hold SHIFT to disable carry. SHIFT-right click to Zero Right.";
    case TuningCanvas::TUNING_HOVER_BW:
        return "Click, wheel or drag(left/right) a digit to change bandwidth; SPACE for direct input.  Hold SHIFT to disable carry. SHIFT-right click to Zero Right.";
    case TuningCanvas::TUNING_HOVER_CENTER:
        return "Click, wheel or drag(left/right) a digit to change center frequency; SPACE for direct input.  Hold SHIFT to disable carry. SHIFT-right click to Zero Right.";
    case TuningCanvas::TUNING_HOVER_PPM:
        return "Click, wheel or drag(left/right) a digit to change device PPM offset.  Hold SHIFT to disable carry.";
    case TuningCanvas::TUNING_HOVER_NONE:
    default:
        return "";
    }
}

} // namespace

TuningCanvas::TuningCanvas(wxWindow *parent) : BgfxCanvas(parent) {
}

TuningCanvas::~TuningCanvas() = default;

void TuningCanvas::setHelpTip(std::string tip) {
    view.helpTip = tip;
}

void TuningCanvas::setHalfBand(bool hb) {
    view.halfBand = hb;
    Refresh();
}

void TuningCanvas::OnKeyDown(wxKeyEvent &event) {
    view.shiftDown = event.ShiftDown();
    view.altDown = event.AltDown();
    if (event.GetKeyCode() == WXK_SPACE) {
        if (view.hoverState == TUNING_HOVER_CENTER || view.hoverState == TUNING_HOVER_FREQ) {
            wxGetApp().showFrequencyInput(FrequencyDialog::FDIALOG_TARGET_DEFAULT);
        } else if (view.hoverState == TUNING_HOVER_BW) {
            wxGetApp().showFrequencyInput(FrequencyDialog::FDIALOG_TARGET_BANDWIDTH);
        }
    }
}

void TuningCanvas::OnKeyUp(wxKeyEvent &event) {
    view.shiftDown = event.ShiftDown();
    view.altDown = event.AltDown();
}

TuningCanvas::ActiveState TuningCanvas::getHoverState() {
    return view.hoverState;
}

void TuningCanvas::StepTuner(
    ActiveState state, TuningDirection tuningDir, int digit, bool preventCarry, bool zeroOut) {
    auto activeDemod = wxGetApp().getDemodMgr().getCurrentModem();
    if (state == TUNING_HOVER_FREQ && activeDemod) {
        long long demod_freq = activeDemod->getFrequency();
        long long diff = abs(wxGetApp().getFrequency() - demod_freq);
        TunerStep::StepInput step;
        step.value = demod_freq;
        step.digit = digit;
        step.up = (tuningDir == TUNING_DIRECTION_UP);
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.bottom = view.bottom;
        demod_freq = TunerStep::stepDigit(step);
        long long viewCenter = 0;
        long long viewBandwidth = 0;
        if (wxGetApp().getMainViewWindow(viewCenter, viewBandwidth)) {
            demod_freq = pinDemodCenter(demod_freq, viewCenter, viewBandwidth);
        }
        if (wxGetApp().getSampleRate() / 2 < diff) {
            wxGetApp().setFrequency(demod_freq);
        }
        activeDemod->setTracking(true);
        activeDemod->setFollow(true);
        activeDemod->setFrequency(demod_freq);
        if (activeDemod->isDeltaLock()) {
            activeDemod->setDeltaLockOfs(deltaLockOffset(activeDemod->getFrequency(), wxGetApp().getFrequency()));
        }
        activeDemod->updateLabel(demod_freq);
    }
    if (state == TUNING_HOVER_BW) {
        long nbw = wxGetApp().getDemodMgr().getLastBandwidth();
        TunerStep::StepInput step;
        step.value = nbw;
        step.digit = digit;
        step.up = (tuningDir == TUNING_DIRECTION_UP);
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.doubleAmount = view.halfBand;
        step.bottom = view.bottom;
        nbw = (long)TunerStep::stepDigit(step);
        if (nbw > CHANNELIZER_RATE_MAX) {
            nbw = CHANNELIZER_RATE_MAX;
        }
        wxGetApp().getDemodMgr().setLastBandwidth(nbw);
        if (activeDemod) {
            activeDemod->setBandwidth(wxGetApp().getDemodMgr().getLastBandwidth());
        }
    }
    if (state == TUNING_HOVER_CENTER) {
        long long ctr = wxGetApp().getFrequency();
        TunerStep::StepInput step;
        step.value = ctr;
        step.digit = digit;
        step.up = (tuningDir == TUNING_DIRECTION_UP);
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.bottom = view.bottom;
        ctr = TunerStep::stepDigit(step);
        wxGetApp().setFrequency(ctr);
    }
    if (state == TUNING_HOVER_PPM) {
        TunerStep::StepInput step;
        step.value = view.currentPPM;
        step.digit = digit;
        step.up = (tuningDir == TUNING_DIRECTION_UP);
        step.preventCarry = preventCarry;
        step.bottom = view.bottom;
        view.currentPPM = (int)TunerStep::stepDigit(step);
        if (view.currentPPM > 2000) {
            view.currentPPM = 2000;
        }
        if (view.currentPPM < -2000) {
            view.currentPPM = -2000;
        }
        wxGetApp().setPPM(view.currentPPM);
        wxGetApp().notifyMainUIOfDeviceChange();
    }
}

void TuningCanvas::updateHover(float mx01, float my01) {
    if (!view.mouseDown) {
        view.top = my01 >= 0.5f;
        view.bottom = my01 <= 0.5f;
    }
    int index = TunerLayout::digitIndexAt(mx01, kFreqCount, kFreqDP, kFreqW);
    if (index > 0) {
        view.hoverIndex = index;
        view.hoverState = view.altDown ? TUNING_HOVER_PPM : TUNING_HOVER_FREQ;
    }
    if (index == 0) {
        index = TunerLayout::digitIndexAt(mx01, kBwCount, kBwDP, kBwW);
        if (index > 0) {
            view.hoverIndex = index;
            view.hoverState = TUNING_HOVER_BW;
        }
    }
    if (index == 0) {
        index = TunerLayout::digitIndexAt(mx01, kFreqCount, kCenterDP, kCenterW);
        if (index > 0) {
            view.hoverIndex = index;
            view.hoverState = TUNING_HOVER_CENTER;
        }
    }
    if (index == 0) {
        view.hoverIndex = 0;
        view.hoverState = TUNING_HOVER_NONE;
    }
}

void TuningCanvas::frameUpdate() {
    if (view.mouseDown && isMouseInView()) {
        if (view.downState != TUNING_HOVER_NONE) {
            wxSize size = GetClientSize();
            float mx01 = size.x > 0 ? mouseX() / (float)size.x : 0.0f;
            view.dragAccum += 5.0f * (mx01 - view.originMouseX);
            TunerStep::DragSteps drag = TunerStep::dragSteps(view.dragAccum);
            view.dragAccum = drag.rest;
            for (int i = 0; i < drag.up; i++) {
                StepTuner(view.downState, TUNING_DIRECTION_UP, view.downIndex - 1, view.shiftDown, false);
                view.dragging = true;
            }
            for (int i = 0; i < drag.down; i++) {
                StepTuner(view.downState, TUNING_DIRECTION_DOWN, view.downIndex - 1, view.shiftDown, false);
                view.dragging = true;
            }
        } else {
            view.dragAccum = 0.0f;
            view.dragging = false;
        }
    }
    long long liveFreq = 0;
    auto liveDemod = wxGetApp().getDemodMgr().getCurrentModem();
    if (liveDemod != nullptr) {
        liveFreq = liveDemod->getFrequency();
    }
    long long liveBw = wxGetApp().getDemodMgr().getLastBandwidth();
    long long liveCenter = wxGetApp().getFrequency();
    if (view.firstPaintCheck || liveFreq != view.paintedFreq || liveBw != view.paintedBw
        || liveCenter != view.paintedCenter || view.currentPPM != view.paintedPPM || view.altDown != view.paintedAlt
        || view.shiftDown != view.paintedShift) {
        view.paintedFreq = liveFreq;
        view.paintedBw = liveBw;
        view.paintedCenter = liveCenter;
        view.paintedPPM = view.currentPPM;
        view.paintedAlt = view.altDown;
        view.paintedShift = view.shiftDown;
        view.firstPaintCheck = false;
        Refresh();
    }
}

void TuningCanvas::drawView(const BgfxEdges &edges, float mx, float my) {
    wxSize size = GetClientSize();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2((float)size.x, (float)size.y), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Tuner", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImVec2 origin = ImGui::GetWindowPos();
    ImVec2 wsize = ImGui::GetWindowSize();
    bool tunerHovered = isMouseInView() && mx >= 0.0f && my >= 0.0f && mx < wsize.x && my < wsize.y;
    view.mouseMX = (mx - origin.x) / wsize.x;
    view.mouseMY = 1.0f - (my - origin.y) / wsize.y;
    view.shiftDown = isShiftDown();
    view.altDown = isAltDown();
    if (edges.mouseLeft) {
        view.hoverIndex = 0;
        view.hoverState = TUNING_HOVER_NONE;
        view.mouseDown = false;
        view.mouseRightDown = false;
        view.shiftDown = false;
        view.altDown = false;
    } else {
        updateHover(view.mouseMX, view.mouseMY);
    }

    if (edges.leftPressed && tunerHovered && !view.mouseRightDown) {
        view.originMouseX = view.mouseMX;
        view.originMouseY = view.mouseMY;
        view.dragAccum = 0.0f;
        view.downIndex = view.hoverIndex;
        view.downState = view.hoverState;
        view.mouseDown = true;
    }
    if (edges.rightPressed && tunerHovered && !view.mouseDown) {
        view.downIndex = view.hoverIndex;
        view.downState = view.hoverState;
        view.mouseRightDown = true;
    }

    if (tunerHovered && view.hoverState != TUNING_HOVER_NONE && !view.mouseDown && view.hoverIndex != 0) {
        if (edges.wheelRotV != 0) {
            TunerStep::WheelSteps wheel = TunerStep::wheelSteps(view.wheelAccumV, edges.wheelRotV, 120);
            view.wheelAccumV = wheel.rest;
            for (int i = 0, n = wheel.steps > 0 ? wheel.steps : -wheel.steps; i < n; i++) {
                StepTuner(view.hoverState, wheel.steps > 0 ? TUNING_DIRECTION_UP : TUNING_DIRECTION_DOWN,
                    view.hoverIndex - 1, view.shiftDown, false);
            }
        }
        if (edges.wheelRotH != 0) {
            TunerStep::WheelSteps wheel = TunerStep::wheelSteps(view.wheelAccumH, edges.wheelRotH, 120);
            view.wheelAccumH = wheel.rest;
            for (int i = 0, n = wheel.steps > 0 ? wheel.steps : -wheel.steps; i < n; i++) {
                StepTuner(view.hoverState, wheel.steps < 0 ? TUNING_DIRECTION_UP : TUNING_DIRECTION_DOWN,
                    view.hoverIndex - 1, view.shiftDown, false);
            }
        }
    }

    if (edges.leftReleased) {
        if (tunerHovered && view.hoverState != TUNING_HOVER_NONE && !view.dragging && view.downState == view.hoverState
            && view.downIndex == view.hoverIndex) {
            StepTuner(view.hoverState, view.top ? TUNING_DIRECTION_UP : TUNING_DIRECTION_DOWN, view.hoverIndex - 1,
                view.shiftDown, false);
        }
        view.mouseDown = false;
        view.dragging = false;
    }
    if (edges.rightReleased) {
        if (view.shiftDown) {
            if (tunerHovered && view.hoverState != TUNING_HOVER_NONE && !view.dragging
                && view.downState == view.hoverState && view.downIndex == view.hoverIndex) {
                StepTuner(view.hoverState, view.top ? TUNING_DIRECTION_UP : TUNING_DIRECTION_DOWN, view.hoverIndex - 1,
                    false, true);
            }
        } else if (tunerHovered && view.hoverState == TUNING_HOVER_FREQ) {
            int snap = wxGetApp().getFrequencySnap();
            if (view.hoverIndex == 1) {
                wxGetApp().setFrequencySnap(1);
            } else if (view.hoverIndex > 1 && view.hoverIndex < 8) {
                int exp = 1;
                for (int i = 1; i < view.hoverIndex; i++) {
                    exp *= 10;
                }
                wxGetApp().setFrequencySnap((snap == exp) ? 1 : exp);
            }
        }
        view.mouseRightDown = false;
    }

    ColorTheme *theme = ThemeMgr::mgr.currentTheme;
    ThemeColor barLight{theme->tuningBarLight.r, theme->tuningBarLight.g, theme->tuningBarLight.b};
    ThemeColor barDark{theme->tuningBarDark.r, theme->tuningBarDark.g, theme->tuningBarDark.b};
    ThemeColor barUp{theme->tuningBarUp.r, theme->tuningBarUp.g, theme->tuningBarUp.b};
    ThemeColor barDown{theme->tuningBarDown.r, theme->tuningBarDown.g, theme->tuningBarDown.b};
    ThemeColor bg{theme->generalBackground.r, theme->generalBackground.g, theme->generalBackground.b};
    ImU32 textCol = packColor(theme->text.r, theme->text.g, theme->text.b);
    imguiSetClearColor(packClear(bg));

    auto activeDemod = wxGetApp().getDemodMgr().getCurrentModem();
    long long freq = 0;
    if (activeDemod) {
        freq = activeDemod->getFrequency();
    }
    long long bw = wxGetApp().getDemodMgr().getLastBandwidth();
    long long center = wxGetApp().getFrequency();
    int snap = wxGetApp().getFrequencySnap();

    TunerFrame f{ImGui::GetWindowDrawList(), origin.x, origin.y, wsize.x, wsize.y};
    ThemeColor clr = view.top ? barUp : barDown;
    for (const BarGroup &g : kBarGroups) {
        drawBarQuads(f, TunerLayout::tunerBarQuads(g.start, g.end, g.count, g.dp, g.w, true, true),
            g.light ? barLight : barDark, bg);
    }
    if (view.mouseDown) {
        drawDragBar(f, view.originMouseX, view.mouseMX, barDark, barLight, barDark, bg);
    }
    if (view.hoverIndex > 0 && !view.mouseDown) {
        switch (view.hoverState) {
        case TUNING_HOVER_FREQ:
            drawBarQuadsStacked(f,
                TunerLayout::tunerBarQuads(view.hoverIndex, view.hoverIndex, kFreqCount, kFreqDP, kFreqW, view.top,
                    view.bottom),
                clr, regionColorAt(view.hoverIndex, kFreqCount, barLight, barDark), bg);
            break;
        case TUNING_HOVER_BW:
            drawBarQuadsStacked(f,
                TunerLayout::tunerBarQuads(view.hoverIndex, view.hoverIndex, kBwCount, kBwDP, kBwW, view.top,
                    view.bottom),
                clr, regionColorAt(view.hoverIndex, kBwCount, barLight, barDark), bg);
            break;
        case TUNING_HOVER_CENTER:
            drawBarQuadsStacked(f,
                TunerLayout::tunerBarQuads(view.hoverIndex, view.hoverIndex, kFreqCount, kCenterDP, kCenterW, view.top,
                    view.bottom),
                clr, regionColorAt(view.hoverIndex, kFreqCount, barLight, barDark), bg);
            break;
        case TUNING_HOVER_PPM:
            drawBarQuadsStacked(f,
                TunerLayout::tunerBarQuads(view.hoverIndex, view.hoverIndex, kFreqCount, kFreqDP, kFreqW, view.top,
                    view.bottom),
                clr, regionColorAt(view.hoverIndex, kFreqCount, barLight, barDark), bg);
            break;
        case TUNING_HOVER_NONE:
            break;
        }
    }

    int fontSize = TunerLayout::tunerFontSize(wsize.x, wsize.y);
    float fontScale = (float)fontSize / 32.0f;
    if (view.altDown) {
        drawTunerDigits(f, view.currentPPM, kFreqCount, kFreqDP, kFreqW, fontScale, textCol);
        drawTunerGrid(f, kFreqCount, kFreqDP, kFreqW);
    } else {
        drawTunerDigits(f, freq, kFreqCount, kFreqDP, kFreqW, fontScale, textCol);
        drawTunerGrid(f, kFreqCount, kFreqDP, kFreqW);
        if (snap != 1) {
            drawSnapBox(f, (int)log10((double)snap), wsize.y);
        }
    }
    drawTunerDigits(f, view.halfBand ? bw / 2 : bw, kBwCount, kBwDP, kBwW, fontScale, textCol);
    drawTunerGrid(f, kBwCount, kBwDP, kBwW);
    drawTunerDigits(f, center, kFreqCount, kCenterDP, kCenterW, fontScale, textCol);
    drawTunerGrid(f, kFreqCount, kCenterDP, kCenterW);
    ImGui::End();
    ImGui::PopStyleVar(2);

    bool haveTip = view.hoverState != TUNING_HOVER_NONE;
    const char *tip = haveTip ? hoverTip(view.hoverState) : "";
    if (!haveTip && !view.helpTip.empty()) {
        tip = view.helpTip.c_str();
        haveTip = true;
    }
    if (haveTip && tip != view.lastTip) {
        view.lastTip = tip;
        wxGetApp().getAppFrame()->setStatusText(this, view.lastTip);
    }
}

void TuningCanvas::onEnterView() {
    view.hoverIndex = 0;
    view.hoverState = TUNING_HOVER_NONE;
    view.currentPPM = wxGetApp().getPPM();
    SetCursor(wxCURSOR_ARROW);
#ifdef _WIN32
    if (wxGetApp().getAppFrame()->canFocus()) {
        SetFocus();
    }
#endif
}

void TuningCanvas::onLeaveView() {
    SetCursor(wxCURSOR_CROSS);
    view.hoverIndex = 0;
    view.hoverState = TUNING_HOVER_NONE;
}
