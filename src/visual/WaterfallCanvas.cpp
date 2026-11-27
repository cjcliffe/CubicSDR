// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "WaterfallCanvas.h"

#include "wx/string.h"

#include <imgui.h>

#include "DemodMarkers.h"
#include "WaterfallDrag.h"
#include "WaterfallHover.h"
#include "TuneRange.h"
#include "GLFont.h"
#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include "AppFrame.h"
#include "SpectrumCanvas.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef USE_HAMLIB
#include "RigThread.h"
#endif

namespace {

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

void drawCenterText(const SpecFrame &f, const char *text, float cxPx, float cyPx, float fontPx, ImU32 col) {
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
        f.dl->AddText(ImVec2(x, y), col, p, p + bytes);
        x += adv;
        p += bytes;
    }
    ImGui::PopFont();
}

void drawCenterWText(
    const SpecFrame &f, const std::wstring &text, float cxPx, float cyPx, float fontPx, ImU32 col) {
    drawCenterText(f, wxString(text).ToUTF8().data(), cxPx, cyPx, fontPx, col);
}

} // namespace

WaterfallCanvas::WaterfallCanvas(wxWindow *parent, const std::string &windowName)
    : BgfxCanvas(parent), spectrumCanvas(nullptr), viewName(windowName), activeTheme(nullptr), texReady(false),
      isView(false), centerFreq(0), bandwidth(0), lastBandwidth(0), dragState(WF_DRAG_NONE),
      nextDragState(WF_DRAG_NONE), hoverDemod(nullptr), fft_size(0), new_fft_size(0), waterfall_lines(0),
      dragOfs(0), mouseZoom(1.0f), zoom(1.0f), freqMoving(false), freqMove(0.0), hoverAlpha(1.0f), scaleMove(0.0f),
      rightDragDist(0.0f), midZooming(false), midZoomStartY(0), dragBW(0), shiftDown(false), altDown(false),
      ctrlDown(false), leftDown(false), rightDown(false), originX01(0.0f), originY01(0.0f), lastX01(0.0f),
      lastY01(0.0f), lastDragX01(0.0f), mouseX01(0.0f), minBandwidth(30000), fft_size_changed(false) {
    tex[0] = BGFX_INVALID_HANDLE;
    tex[1] = BGFX_INVALID_HANDLE;
    pacer.setLinesPerSecond(DEFAULT_WATERFALL_LPS);
    SetCursor(wxCURSOR_CROSS);
}

WaterfallCanvas::~WaterfallCanvas() {
    if (bgfx::isValid(tex[0])) {
        bgfx::destroy(tex[0]);
    }
    if (bgfx::isValid(tex[1])) {
        bgfx::destroy(tex[1]);
    }
}

void WaterfallCanvas::setup(unsigned int fft_size_in, int waterfall_lines_in) {
    if (fft_size == fft_size_in && waterfall_lines_in == waterfall_lines) {
        return;
    }
    fft_size = fft_size_in;
    waterfall_lines = waterfall_lines_in;

    ring.setup(fft_size, waterfall_lines);
    texReady = false;
    lastFrameTime = std::chrono::steady_clock::now();
}

void WaterfallCanvas::setFFTSize(unsigned int fft_size_in) {
    if (fft_size_in == fft_size) {
        return;
    }
    new_fft_size = fft_size_in;
    fft_size_changed.store(true);
}

WaterfallCanvas::DragState WaterfallCanvas::getDragState() {
    return dragState;
}

WaterfallCanvas::DragState WaterfallCanvas::getNextDragState() {
    return nextDragState;
}

void WaterfallCanvas::attachSpectrumCanvas(SpectrumCanvas *canvas_in) {
    spectrumCanvas = canvas_in;
}

SpectrumVisualDataQueuePtr WaterfallCanvas::getVisualDataQueue() {
    return visualDataQueue;
}

DemodulatorInstancePtr WaterfallCanvas::getHoverDemod() {
    return hoverDemod;
}

void WaterfallCanvas::clearHoverDemod() {
    hoverDemod = nullptr;
}

float WaterfallCanvas::hoverX01() const {
    return mouseX01;
}

bool WaterfallCanvas::isShiftHeld() const {
    return shiftDown || isShiftDown();
}

void WaterfallCanvas::setView(long long center_freq_in, long long bandwidth_in) {
    isView = true;
    centerFreq = center_freq_in;
    bandwidth = bandwidth_in;
    lastBandwidth = 0;
}

void WaterfallCanvas::disableView() {
    isView = false;
    centerFreq = wxGetApp().getFrequency();
    bandwidth = wxGetApp().getSampleRate();
    lastBandwidth = 0;
}

bool WaterfallCanvas::getViewState() const {
    return isView;
}

void WaterfallCanvas::setCenterFrequency(long long center_freq_in) {
    centerFreq = center_freq_in;
}

long long WaterfallCanvas::getCenterFrequency() const {
    if (isView) {
        return centerFreq;
    }
    return wxGetApp().getFrequency();
}

void WaterfallCanvas::setBandwidth(long long bandwidth_in) {
    bandwidth = bandwidth_in;
}

long long WaterfallCanvas::getBandwidth() const {
    if (isView) {
        return bandwidth;
    }
    return wxGetApp().getSampleRate();
}

long long WaterfallCanvas::getFrequencyAt(float x) const {
    return WaterfallDrag::freqAtX(x, getCenterFrequency(), getBandwidth());
}

long long WaterfallCanvas::getFrequencyAt(float x, long long iqCenterFreq, long long iqBandwidth) const {
    return WaterfallDrag::freqAtX(x, iqCenterFreq, iqBandwidth);
}

void WaterfallCanvas::frameUpdate() {
    processInputQueue();
    Refresh();
}

void WaterfallCanvas::processInputQueue() {
    std::lock_guard<std::mutex> lock(tex_update);

    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> dt = now - lastFrameTime;
    lastFrameTime = now;

    size_t depth = visualDataQueue ? visualDataQueue->size() : 0;
    WaterfallPacer::Plan plan = pacer.plan(dt.count(), depth);

    SpectrumVisualDataPtr dropData;
    for (int d = 0; d < plan.dropBefore && visualDataQueue; d++) {
        if (!visualDataQueue->try_pop(dropData)) {
            break;
        }
    }

    for (int s = 0; s < plan.insert; s++) {
        SpectrumVisualDataPtr vData;
        if (!visualDataQueue || !visualDataQueue->try_pop(vData)) {
            break;
        }

        if (vData) {
            if (vData->spectrum_points.size() == fft_size * 2) {
                ring.setPoints(vData->spectrum_points);
            }
            ring.step();
        }
    }

    for (int d = 0; d < plan.dropAfter && visualDataQueue; d++) {
        if (!visualDataQueue->try_pop(dropData)) {
            break;
        }
    }

    updateTextures();
}

void WaterfallCanvas::updateTextures() {
    if (!BgfxCanvas::isBackendReady()) {
        return;
    }
    unsigned int half = fft_size / 2;

    if (!texReady) {
        for (int i = 0; i < 2; i++) {
            if (bgfx::isValid(tex[i])) {
                bgfx::destroy(tex[i]);
                tex[i] = BGFX_INVALID_HANDLE;
            }
        }
        if (half == 0 || waterfall_lines <= 0) {
            return;
        }
        for (int i = 0; i < 2; i++) {
            tex[i] = bgfx::createTexture2D(
                (uint16_t)half, (uint16_t)waterfall_lines, false, 1, bgfx::TextureFormat::RGBA8, 0, NULL);
            shadow[i].assign((size_t)half * (size_t)waterfall_lines, 0);
        }
        lut.assign(256 * 4, 0);
        activeTheme = nullptr;
        texReady = true;
        ring.setReady(true);
    }

    if (activeTheme != ThemeMgr::mgr.currentTheme) {
        activeTheme = ThemeMgr::mgr.currentTheme;
        std::vector<float> &lr = activeTheme->waterfallGradient.getRed();
        std::vector<float> &lg = activeTheme->waterfallGradient.getGreen();
        std::vector<float> &lb = activeTheme->waterfallGradient.getBlue();
        for (int i = 0; i < 256; i++) {
            lut[(size_t)i * 4] = clampByte(lr[(size_t)i]);
            lut[(size_t)i * 4 + 1] = clampByte(lg[(size_t)i]);
            lut[(size_t)i * 4 + 2] = clampByte(lb[(size_t)i]);
            lut[(size_t)i * 4 + 3] = 255;
        }
        for (int t = 0; t < 2; t++) {
            staging.resize((size_t)half * (size_t)waterfall_lines * 4);
            for (int i = 0; i < (int)half * waterfall_lines; i++) {
                const uint8_t *entry = &lut[(size_t)shadow[t][(size_t)i] * 4];
                staging[(size_t)i * 4] = entry[0];
                staging[(size_t)i * 4 + 1] = entry[1];
                staging[(size_t)i * 4 + 2] = entry[2];
                staging[(size_t)i * 4 + 3] = 255;
            }
            const bgfx::Memory *mem = bgfx::copy(staging.data(), (uint32_t)staging.size());
            bgfx::updateTexture2D(
                tex[t], 0, 0, 0, 0, (uint16_t)half, (uint16_t)waterfall_lines, mem);
        }
    }

    WaterfallRing::UploadPlan up = ring.planUpload();
    for (const auto &op : up.ops) {
        const unsigned char *src = ring.reversedLine(op.texture) + op.srcOffset;
        std::memcpy(&shadow[op.texture][(size_t)op.y * (size_t)half], src,
            (size_t)op.height * (size_t)half);
        staging.resize((size_t)half * (size_t)op.height * 4);
        for (int i = 0; i < (int)half * op.height; i++) {
            const uint8_t *entry = &lut[(size_t)src[(size_t)i] * 4];
            staging[(size_t)i * 4] = entry[0];
            staging[(size_t)i * 4 + 1] = entry[1];
            staging[(size_t)i * 4 + 2] = entry[2];
            staging[(size_t)i * 4 + 3] = 255;
        }
        const bgfx::Memory *mem = bgfx::copy(staging.data(), (uint32_t)staging.size());
        bgfx::updateTexture2D(tex[op.texture], 0, 0, 0, (uint16_t)op.y, (uint16_t)half,
            (uint16_t)op.height, mem);
    }
    ring.commitUpload(up);
}

void WaterfallCanvas::setLinesPerSecond(int lps) {
    std::lock_guard<std::mutex> lock(tex_update);

    pacer.setLinesPerSecond(lps);

    visualDataQueue->flush();
}

void WaterfallCanvas::setMinBandwidth(int min) {
    minBandwidth = min;
}

void WaterfallCanvas::updateCenterFrequency(long long freq) {
    if (isView) {
        setView(freq, getBandwidth());
        if (spectrumCanvas) {
            spectrumCanvas->setView(freq, getBandwidth());
        }

        long long minFreq = wxGetApp().getFrequency() - (wxGetApp().getSampleRate() / 2);
        long long maxFreq = wxGetApp().getFrequency() + (wxGetApp().getSampleRate() / 2);

        if (freq - bandwidth / 2 < minFreq) {
            wxGetApp().setFrequency(wxGetApp().getFrequency() - (minFreq - (freq - bandwidth / 2)));
        }
        if (freq + bandwidth / 2 > maxFreq) {
            wxGetApp().setFrequency(wxGetApp().getFrequency() + ((freq + bandwidth / 2) - maxFreq));
        }

        long long pinned =
            pinViewCenter(freq, getBandwidth(), wxGetApp().getFrequency(), wxGetApp().getSampleRate());
        if (pinned != freq) {
            freq = pinned;
            setView(freq, getBandwidth());
            if (spectrumCanvas) {
                spectrumCanvas->setView(freq, getBandwidth());
            }
        }
    } else {
        if (spectrumCanvas) {
            spectrumCanvas->setCenterFrequency(freq);
        }
        wxGetApp().setFrequency(freq);
    }
}

void WaterfallCanvas::updateHoverState(float mx01) {
    long long freqPos = getFrequencyAt(mx01);

    auto demodsHover = wxGetApp().getDemodMgr().getDemodulatorsAt(freqPos, 15000);

    hoverDemod = nullptr;

    if (altDown || isAltDown()) {
        nextDragState = WF_DRAG_RANGE;
        if (shiftDown || isShiftDown()) {
            wxGetApp().getAppFrame()->setStatusText(
                this, "Click and drag to create a new demodulator by range.");
        } else {
            wxGetApp().getAppFrame()->setStatusText(this, "Click and drag to set the current demodulator range.");
        }
    } else if (!demodsHover.empty() && !(shiftDown || isShiftDown())) {
        std::vector<WaterfallHover::HoverCandidate> candidates;
        for (auto demod : demodsHover) {
            WaterfallHover::HoverCandidate cand;
            cand.freqHz = demod->getFrequency();
            cand.bandwidthHz = demod->getBandwidth();
            cand.sideband = DemodMarkers::sidebandForType(demod->getDemodulatorType());
            candidates.push_back(cand);
        }
        WaterfallHover::HoverPick pick =
            WaterfallHover::pickHover(candidates, freqPos, getBandwidth(), wxGetApp().getSampleRate());

        if (pick.zone == WaterfallHover::DragZone::None) {
            nextDragState = WF_DRAG_NONE;
            SetCursor(wxCURSOR_CROSS);
            return;
        }

        hoverDemod = demodsHover[(size_t)pick.index];

        if (pick.zone == WaterfallHover::DragZone::Frequency) {
            SetCursor(wxCURSOR_SIZING);
            nextDragState = WF_DRAG_FREQUENCY;

            wxGetApp().getAppFrame()->setStatusText(this,
                "Drag to change frequency. SPACE/0-9 direct input. [, ] nudge, M mute, V pin, DEL delete, C "
                "center, E edit label, R record, TAB cycle selection.");
        } else {
            if (pick.zone == WaterfallHover::DragZone::BandwidthLeft) {
                nextDragState = WF_DRAG_BANDWIDTH_LEFT;
                SetCursor(wxCURSOR_SIZEWE);
            } else if (pick.zone == WaterfallHover::DragZone::BandwidthRight) {
                nextDragState = WF_DRAG_BANDWIDTH_RIGHT;
                SetCursor(wxCURSOR_SIZEWE);
            }

            wxGetApp().getAppFrame()->setStatusText(this,
                "Drag to change bandwidth. SPACE/0-9 direct input. [, ] nudge, M mute, V pin, DEL delete, C "
                "center, E edit label, R record, TAB cycle selection.");
        }
    } else {
        SetCursor(wxCURSOR_CROSS);
        nextDragState = WF_DRAG_NONE;
        if (shiftDown || isShiftDown()) {
            wxGetApp().getAppFrame()->setStatusText(this, "Click to create a new demodulator or hold ALT to drag new range.");
        } else {
            wxGetApp().getAppFrame()->setStatusText(this,
                "Click sets demodulator frequency, ALT-drag draws a range, SHIFT-click creates new. Arrows/wheel "
                "navigate/zoom, C centers, TAB cycles selection. Right-drag or SHIFT+UP/DOWN adjusts visual gain, "
                "right-click resets it. Middle-drag up/down to glide-zoom. Shift-R/M record/mute all.");
        }
    }
}

void WaterfallCanvas::onEnterView() {
    SetCursor(wxCURSOR_CROSS);
#ifdef _WIN32
    if (wxGetApp().getAppFrame()->canFocus()) {
        SetFocus();
    }
#endif
}

void WaterfallCanvas::onLeaveView() {
    SetCursor(wxCURSOR_CROSS);
    hoverDemod = nullptr;
    hoverAlpha = 0.0f;
    mouseZoom = 1.0f;
    leftDown = false;
    rightDown = false;
    if (midZooming) {
        midZooming = false;
        zoom = 1.0f;
    }
}

void WaterfallCanvas::setMouseZoom(float mouseZoom_in) {
    mouseZoom = mouseZoom_in;
}

void WaterfallCanvas::OnKeyUp(wxKeyEvent& event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
    switch (event.GetKeyCode()) {
    case WXK_UP:
    case WXK_NUMPAD_UP:
        scaleMove = 0.0;
        zoom = 1.0;
        if (mouseZoom != 1.0) {
            mouseZoom = 0.95f;
        }
        break;
    case WXK_DOWN:
    case WXK_NUMPAD_DOWN:
        scaleMove = 0.0;
        zoom = 1.0;
        if (mouseZoom != 1.0) {
            mouseZoom = 1.05f;
        }
        break;
    case WXK_LEFT:
    case WXK_NUMPAD_LEFT:
    case WXK_RIGHT:
    case WXK_NUMPAD_RIGHT:
        freqMoving = false;
        break;
    }
}

void WaterfallCanvas::OnKeyDown(wxKeyEvent& event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    long long originalFreq = getCenterFrequency();
    long long freq = originalFreq;

    switch (event.GetKeyCode()) {
    case WXK_UP:
    case WXK_NUMPAD_UP:
        if (!shiftDown) {
            mouseZoom = 1.0;
            zoom = 0.95f;
        } else {
            scaleMove = 1.0;
        }
        break;
    case WXK_DOWN:
    case WXK_NUMPAD_DOWN:
        if (!shiftDown) {
            mouseZoom = 1.0;
            zoom = 1.05f;
        } else {
            scaleMove = -1.0;
        }
        break;
    case WXK_RIGHT:
    case WXK_NUMPAD_RIGHT:
        if (isView) {
            freqMove = shiftDown ? 5.0 : 1.0;
            freqMoving = true;
        } else {
            freq += shiftDown ? (getBandwidth() * 10) : (getBandwidth() / 2);
        }
        break;
    case WXK_LEFT:
    case WXK_NUMPAD_LEFT:
        if (isView) {
            freqMove = shiftDown ? -5.0 : -1.0;
            freqMoving = true;
        } else {
            freq -= shiftDown ? (getBandwidth() * 10) : (getBandwidth() / 2);
        }
        break;
    case WXK_SPACE:
        wxGetApp().showFrequencyInput();
        break;
    case 'E':
        {
            DemodulatorInstancePtr labelDemod =
                hoverDemod ? hoverDemod : wxGetApp().getDemodMgr().getCurrentModem();
            wxGetApp().showLabelInput(labelDemod);
        }
        break;
    case 'C':
        if (hoverDemod) {
            wxGetApp().setFrequency(hoverDemod->getFrequency());
        } else if (isMouseInView()) {
            long long nfreq = getFrequencyAt(mouseX01);

            int snap = wxGetApp().getFrequencySnap();

            if (snap > 1) {
                nfreq = (long long)(roundf((float)nfreq / (float)snap) * (float)snap);
            }

            wxGetApp().setFrequency(nfreq);
        }
#ifdef USE_HAMLIB
        if (wxGetApp().rigIsActive()
            && (!wxGetApp().getRigThread()->getControlMode() || wxGetApp().getRigThread()->getCenterLock())) {
            wxGetApp().getRigThread()->setFrequency(wxGetApp().getFrequency(), true);
        }
#endif
        break;
    default:
        event.Skip();
        return;
    }

    if (freq != originalFreq) {
        updateCenterFrequency(freq);
    }
}

void WaterfallCanvas::drawView(const BgfxEdges &edges, float mx, float my) {
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

    if (edges.mouseLeft) {
        hoverDemod = nullptr;
        hoverAlpha = 0.0f;
        mouseZoom = 1.0f;
        leftDown = false;
        rightDown = false;
        if (midZooming) {
            midZooming = false;
            zoom = 1.0f;
        }
    }

    if (edges.leftPressed && hovered && !rightDown) {
        leftDown = true;
        originX01 = mx01;
        originY01 = my01;
        lastDragX01 = mx01;
        updateHoverState(mx01);
        dragState = nextDragState;
        wxGetApp().getDemodMgr().updateLastState();

        if (dragState && dragState != WF_DRAG_RANGE) {
            wxGetApp().getDemodMgr().setActiveDemodulator(
                hoverDemod ? hoverDemod : wxGetApp().getDemodMgr().getCurrentModem());
            auto demod = wxGetApp().getDemodMgr().getCurrentModem();
            if (demod) {
                dragOfs = (int)WaterfallDrag::dragOffset(mx01, getBandwidth(), getCenterFrequency(),
                    demod->getFrequency());
                dragBW = demod->getBandwidth();
            }
        }
    }

    auto demod = wxGetApp().getDemodMgr().getCurrentModem();
    if (leftDown && hovered && demod != nullptr) {
        if (dragState == WF_DRAG_BANDWIDTH_LEFT || dragState == WF_DRAG_BANDWIDTH_RIGHT) {
            int currentBW = WaterfallDrag::bandwidthDragStep(mx01 - lastDragX01, getBandwidth(),
                dragState == WF_DRAG_BANDWIDTH_LEFT, dragBW, MIN_BANDWIDTH, CHANNELIZER_RATE_MAX);
            demod->setBandwidth(currentBW);
            dragBW = currentBW;
            lastDragX01 = mx01;
        } else if (dragState == WF_DRAG_FREQUENCY) {
            long long bwTarget = getFrequencyAt(mx01) - dragOfs;
            long long currentFreq = demod->getFrequency();
            long long bwDiff =
                WaterfallDrag::snapDelta(bwTarget - currentFreq, wxGetApp().getFrequencySnap());

            if (bwDiff) {
                demod->setFrequency(pinDemodCenter(currentFreq + bwDiff, getCenterFrequency(), getBandwidth()));
                if (demod->isDeltaLock()) {
                    demod->setDeltaLockOfs(deltaLockOffset(demod->getFrequency(), wxGetApp().getFrequency()));
                }
                currentFreq = demod->getFrequency();
                demod->updateLabel(currentFreq);
            }
            lastDragX01 = mx01;
        }
    }

    if (edges.leftReleased) {
        wxGetApp().getDemodMgr().updateLastState();

        bool relIsNew = (shiftDown || isShiftDown()) || (wxGetApp().getDemodMgr().getCurrentModem() == nullptr)
            || (wxGetApp().getDemodMgr().getCurrentModem() && !wxGetApp().getDemodMgr().getCurrentModem()->isActive());

        DemodulatorInstancePtr relDemod =
            relIsNew ? nullptr : wxGetApp().getDemodMgr().getCurrentModem();

        DemodulatorMgr *mgr = &wxGetApp().getDemodMgr();

        if ((mx01 - originX01) == 0.0f && (my01 - originY01) == 0.0f) {
            long long freq = WaterfallDrag::clickPlaceFreq(mx01, getCenterFrequency(), getBandwidth(),
                relDemod != nullptr, relDemod ? relDemod->getFrequency() : 0, wxGetApp().getFrequencySnap());

            if (dragState == WF_DRAG_NONE) {
                if (!relIsNew && !wxGetApp().getDemodMgr().getDemodulators().empty()) {
                    mgr->updateLastState();
                    relDemod = wxGetApp().getDemodMgr().getCurrentModem();
                } else {
                    relIsNew = true;
                    relDemod = wxGetApp().getDemodMgr().newThread();
                    relDemod->setFrequency(freq);

                    relDemod->setDemodulatorType(mgr->getLastDemodulatorType());
                    relDemod->setBandwidth(mgr->getLastBandwidth());
                    relDemod->setSquelchLevel(mgr->getLastSquelchLevel());
                    relDemod->setSquelchEnabled(mgr->isLastSquelchEnabled());
                    relDemod->setGain(mgr->getLastGain());
                    relDemod->setMuted(mgr->isLastMuted());
                    if (mgr->getLastDeltaLock()) {
                        relDemod->setDeltaLock(true);
                        relDemod->setDeltaLockOfs(deltaLockOffset(freq, wxGetApp().getFrequency()));
                    } else {
                        relDemod->setDeltaLock(false);
                    }
                    relDemod->writeModemSettings(mgr->getLastModemSettings(mgr->getLastDemodulatorType()));
                    relDemod->run();

                    wxGetApp().notifyDemodulatorsChanged();
                }

                if (relDemod) {
                    relDemod->updateLabel(freq);
                    relDemod->setFrequency(freq);
                    if (relDemod->isDeltaLock()) {
                        relDemod->setDeltaLockOfs(
                            deltaLockOffset(relDemod->getFrequency(), wxGetApp().getFrequency()));
                    }

                    if (relIsNew) {
                        wxGetApp().getAppFrame()->setStatusText(
                            "New demodulator at frequency: %s", (int)freq);
                    } else {
                        wxGetApp().getAppFrame()->setStatusText(
                            "Moved demodulator to frequency: %s", (int)freq);
                    }

                    wxGetApp().getDemodMgr().setActiveDemodulator(relDemod);
                    SetCursor(wxCURSOR_SIZING);
                    nextDragState = WF_DRAG_FREQUENCY;
                }
                dragState = WF_DRAG_NONE;
            } else {
                if (relDemod) {
                    wxGetApp().getDemodMgr().setActiveDemodulator(relDemod);
                    mgr->updateLastState();
                    relDemod->setTracking(true);
                    nextDragState = WF_DRAG_FREQUENCY;
                } else {
                    nextDragState = WF_DRAG_NONE;
                }
                dragState = WF_DRAG_NONE;
            }
        } else if (dragState == WF_DRAG_RANGE) {
            WaterfallDrag::RangeDrag range = WaterfallDrag::rangeDrag(originX01, mx01, mx01 - originX01,
                DemodMarkers::sidebandForType(mgr->getLastDemodulatorType()), getCenterFrequency(),
                getBandwidth(), wxGetApp().getFrequencySnap(), MIN_BANDWIDTH);

            if (range.bandwidthHz) {
                long long freq = pinDemodCenter(range.freqHz, getCenterFrequency(), getBandwidth());
                unsigned int bw = range.bandwidthHz;

                if (!relIsNew && !wxGetApp().getDemodMgr().getDemodulators().empty()) {
                    mgr->updateLastState();
                    relDemod = wxGetApp().getDemodMgr().getCurrentModem();
                } else {
                    relDemod = wxGetApp().getDemodMgr().newThread();
                    relDemod->setFrequency(freq);
                    relDemod->setDemodulatorType(mgr->getLastDemodulatorType());
                    relDemod->setBandwidth(bw);
                    relDemod->setSquelchLevel(mgr->getLastSquelchLevel());
                    relDemod->setSquelchEnabled(mgr->isLastSquelchEnabled());
                    relDemod->setGain(mgr->getLastGain());
                    relDemod->setMuted(mgr->isLastMuted());
                    if (mgr->getLastDeltaLock()) {
                        relDemod->setDeltaLock(true);
                        relDemod->setDeltaLockOfs(deltaLockOffset(freq, wxGetApp().getFrequency()));
                    } else {
                        relDemod->setDeltaLock(false);
                    }
                    relDemod->writeModemSettings(mgr->getLastModemSettings(mgr->getLastDemodulatorType()));

                    relDemod->run();

                    wxGetApp().notifyDemodulatorsChanged();
                }

                if (relDemod != nullptr) {
                    wxGetApp().getAppFrame()->setStatusText("New demodulator at frequency: %s", (int)freq);

                    relDemod->updateLabel(freq);
                    relDemod->setFrequency(freq);
                    if (relDemod->isDeltaLock()) {
                        relDemod->setDeltaLockOfs(
                            deltaLockOffset(relDemod->getFrequency(), wxGetApp().getFrequency()));
                    }
                    relDemod->setBandwidth(bw);
                    mgr->setActiveDemodulator(relDemod);
                    mgr->updateLastState();
                }
            }

            dragState = WF_DRAG_NONE;
        } else {
            dragState = WF_DRAG_NONE;
        }

        leftDown = false;
    }

    if (edges.rightPressed && hovered && !leftDown) {
        rightDown = true;
        rightDragDist = 0.0f;
        lastY01 = my01;
        SetCursor(wxCURSOR_SIZENS);
    }
    if (rightDown && hovered && spectrumCanvas) {
        float dy = my01 - lastY01;
        spectrumCanvas->updateScaleFactorFromYMove(dy);
        rightDragDist += fabsf(dy);
        lastY01 = my01;
    }
    if (edges.rightReleased) {
        if (rightDragDist < 0.01f && spectrumCanvas) {
            spectrumCanvas->resetScaleFactorFromClick();
        }
        rightDown = false;
        SetCursor(wxCURSOR_CROSS);
    }

    if (edges.middlePressed && hovered) {
        midZooming = true;
        midZoomStartY = (int)my;
        SetCursor(wxCURSOR_SIZENS);
    }
    if (midZooming) {
        int height = (int)wsize.y;
        if (height > 0) {
            zoom = WaterfallDrag::glideZoom(midZoomStartY, (int)my, height);
        }
    }
    if (edges.middleReleased) {
        if (midZooming) {
            midZooming = false;
            zoom = 1.0f;
        }
        SetCursor(wxCURSOR_CROSS);
    }

    if (edges.wheelRotV != 0 && hovered) {
        mouseZoom = WaterfallDrag::wheelZoom(edges.wheelRotV, 120);
    }

    if (hovered && !leftDown && !rightDown && !midZooming && (mx01 != lastX01 || my01 != lastY01)) {
        lastX01 = mx01;
        lastY01 = my01;
        updateHoverState(mx01);
    }

    long double currentZoom = zoom;

    if (mouseZoom != 1) {
        currentZoom = mouseZoom;
        mouseZoom = mouseZoom + (1.0 - mouseZoom) * 0.2;
        if (fabs(mouseZoom - 1.0) < 0.01) {
            mouseZoom = 1;
        }
    }

    if (scaleMove != 0) {
        SpectrumVisualProcessor *sp = wxGetApp().getSpectrumProcessor();
        FFTVisualDataThread *wdt = wxGetApp().getAppFrame()->getWaterfallDataThread();
        SpectrumVisualProcessor *wp = wdt->getProcessor();
        float factor = sp->getScaleFactor();

        factor += scaleMove * 0.02;

        if (factor < 0.25) {
            factor = 0.25;
        }
        if (factor > 10.0) {
            factor = 10.0;
        }

        sp->setScaleFactor(factor);
        wp->setScaleFactor(factor);
        if (wxGetApp().getDemodSpectrumProcessor()) {
            wxGetApp().getDemodSpectrumProcessor()->setScaleFactor(factor);
        }
    }

    if (freqMove != 0.0) {
        long long newFreq = getCenterFrequency() + (long long)((long double)getBandwidth() * freqMove) * 0.01;

        updateCenterFrequency(newFreq);

        if (!freqMoving) {
            freqMove -= (freqMove * 0.2);
            if (fabs(freqMove) < 0.01) {
                freqMove = 0.0;
            }
        }
    }

    long long bw;
    if (currentZoom != 1) {
        long long freq = wxGetApp().getFrequency();
        bw = getBandwidth();

        double mpos = 0;
        float mouseInView = false;

        if (isMouseInView()) {
            mpos = mouseX01;
            mouseInView = true;
        } else if (spectrumCanvas && spectrumCanvas->isMouseInView()) {
            mpos = spectrumCanvas->hoverX01();
            mouseInView = true;
        }

        if (currentZoom < 1) {
            bw = (long long)ceil((long double)bw * currentZoom);
            if (bw < minBandwidth) {
                bw = minBandwidth;
            }
            if (mouseInView) {
                long long mfreqA = getFrequencyAt((float)mpos, centerFreq, getBandwidth());
                long long mfreqB = getFrequencyAt((float)mpos, centerFreq, bw);
                centerFreq += mfreqA - mfreqB;
            }

            setView(centerFreq, bw);
        } else {
            if (isView) {
                bw = (long long)ceil((long double)bw * currentZoom);

                if (bw >= wxGetApp().getSampleRate()) {
                    disableView();
                    if (spectrumCanvas) {
                        spectrumCanvas->disableView();
                    }
                    bw = wxGetApp().getSampleRate();
                    centerFreq = wxGetApp().getFrequency();
                } else {
                    if (mouseInView) {
                        long long mfreqA = getFrequencyAt((float)mpos, centerFreq, getBandwidth());
                        long long mfreqB = getFrequencyAt((float)mpos, centerFreq, bw);
                        centerFreq += mfreqA - mfreqB;
                        setBandwidth(bw);
                    } else {
                        setBandwidth(bw);
                    }
                }
            }
        }
        if (centerFreq < freq && (centerFreq - bandwidth / 2) < (freq - wxGetApp().getSampleRate() / 2)) {
            centerFreq = (freq - wxGetApp().getSampleRate() / 2) + bandwidth / 2;
        }
        if (centerFreq > freq && (centerFreq + bandwidth / 2) > (freq + wxGetApp().getSampleRate() / 2)) {
            centerFreq = (freq + wxGetApp().getSampleRate() / 2) - bandwidth / 2;
        }

        if (spectrumCanvas) {
            if ((spectrumCanvas->getCenterFrequency() != centerFreq) || (spectrumCanvas->getBandwidth() != bw)) {
                if (getViewState()) {
                    spectrumCanvas->setView(centerFreq, bw);
                } else {
                    spectrumCanvas->disableView();
                    spectrumCanvas->setCenterFrequency(centerFreq);
                    spectrumCanvas->setBandwidth(bw);
                }
            }
        }
    }

    if (fft_size_changed.load()) {
        fft_size = new_fft_size;
        ring.setup(fft_size, waterfall_lines);
        texReady = false;
        fft_size_changed.store(false);
    }

    std::lock_guard<std::mutex> lock(tex_update);

    imguiSetClearColor(0x000000FF);
    ColorTheme *currentTheme = ThemeMgr::mgr.currentTheme;

    if (texReady) {
        int halfW = (int)fft_size / 2;
        int lines = waterfall_lines;
        int offset = ring.rowOffset();
        float u0 = 0.5f / (float)halfW;
        float u1 = 1.0f - 0.5f / (float)halfW;
        float splitY = f.oy + wsize.y * (float)(lines - offset) / (float)lines;
        float vTopA = ((float)offset + 0.5f) / (float)lines;
        float vBotA = ((float)lines - 0.5f) / (float)lines;
        ImTextureID texId0 = imguiTextureId(tex[0]);
        ImTextureID texId1 = imguiTextureId(tex[1]);
        f.dl->AddImage(texId0, ImVec2(f.ox, f.oy), ImVec2(f.ox + wsize.x * 0.5f + 0.5f, splitY),
            ImVec2(u0, vTopA), ImVec2(u1, vBotA));
        f.dl->AddImage(texId1, ImVec2(f.ox + wsize.x * 0.5f - 0.5f, f.oy), ImVec2(f.ox + wsize.x, splitY),
            ImVec2(u0, vTopA), ImVec2(u1, vBotA));
        if (offset > 0) {
            float vTopB = 0.5f / (float)lines;
            float vBotB = ((float)offset - 0.5f) / (float)lines;
            f.dl->AddImage(texId0, ImVec2(f.ox, splitY), ImVec2(f.ox + wsize.x * 0.5f + 0.5f, f.oy + wsize.y),
                ImVec2(u0, vTopB), ImVec2(u1, vBotB));
            f.dl->AddImage(texId1, ImVec2(f.ox + wsize.x * 0.5f - 0.5f, splitY),
                ImVec2(f.ox + wsize.x, f.oy + wsize.y), ImVec2(u0, vTopB), ImVec2(u1, vBotB));
        }
    }

    auto demods = wxGetApp().getDemodMgr().getDemodulators();
    auto selectedDemodulator = wxGetApp().getDemodMgr().getCurrentModem();

    bool paintIsNew = (shiftDown || isShiftDown())
        || (selectedDemodulator && !selectedDemodulator->isActive());

    bool dimSelection = selectedDemodulator
        && ((hoverDemod && hoverDemod != selectedDemodulator) || (!hoverDemod && paintIsNew));

    int currentBandwidth = (int)getBandwidth();
    long long currentCenterFreq = getCenterFrequency();
    std::string last_type = wxGetApp().getDemodMgr().getLastDemodulatorType();
    int viewW = (int)wsize.x;
    int viewH = (int)wsize.y;

    if (hovered || hoverDemod) {
        hoverAlpha += (1.0f - hoverAlpha) * 0.1f;
        if (hoverAlpha > 1.5f) {
            hoverAlpha = 1.5f;
        }
        if (nextDragState == WF_DRAG_RANGE) {
            float width = 1.0f / wsize.x;
            float rangeWidth = mx01 - originX01;
            float centerPos;

            if (leftDown) {
                if (rangeWidth) {
                    width = rangeWidth;
                }
                centerPos = originX01 + width / 2.0f;
            } else {
                centerPos = mx01;
            }

            drawDemodEdges(f.dl, f.ox, f.oy, wsize.x, wsize.y, selectedDemodulator,
                paintIsNew ? currentTheme->waterfallHighlight : currentTheme->waterfallDestroy,
                viewW, viewH, 0.0f);

            if ((last_type == "LSB" || last_type == "USB") && leftDown) {
                centerPos = mx01;
                float p1 = centerPos;
                float p2 = centerPos - width;
                if (p2 < p1) {
                    float tmp = p1;
                    p1 = p2;
                    p2 = tmp;
                }
                ImU32 rangeCol = paintIsNew ? packColor(currentTheme->waterfallNew.r,
                                                  currentTheme->waterfallNew.g, currentTheme->waterfallNew.b,
                                                  0.51f)
                                            : packColor(currentTheme->waterfallHover.r,
                                                  currentTheme->waterfallHover.g, currentTheme->waterfallHover.b,
                                                  0.51f);
                float w1 = last_type == "USB" ? 2.0f : 1.0f;
                float w2 = last_type == "LSB" ? 2.0f : 1.0f;
                f.dl->PushTexture(imguiAdditiveTexture());
                f.dl->AddLine(ImVec2(f.nx((p1 - 0.5f) * 2.0f), f.ny(1.0f)),
                    ImVec2(f.nx((p1 - 0.5f) * 2.0f), f.ny(-1.0f)), rangeCol, w1);
                f.dl->AddLine(ImVec2(f.nx((p2 - 0.5f) * 2.0f), f.ny(1.0f)),
                    ImVec2(f.nx((p2 - 0.5f) * 2.0f), f.ny(-1.0f)), rangeCol, w2);
                f.dl->PopTexture();
            } else {
                float centerNDC = (centerPos - 0.5f) * 2.0f;
                ImU32 rangeCol = paintIsNew ? packColor(currentTheme->waterfallNew.r,
                                                  currentTheme->waterfallNew.g, currentTheme->waterfallNew.b,
                                                  0.51f)
                                            : packColor(currentTheme->waterfallHover.r,
                                                  currentTheme->waterfallHover.g, currentTheme->waterfallHover.b,
                                                  0.51f);
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
            if (selectedDemodulator && !dimSelection) {
                bool lit = (paintIsNew && hoverDemod == nullptr) || (hoverDemod != nullptr);
                drawDemodEdges(f.dl, f.ox, f.oy, wsize.x, wsize.y, selectedDemodulator,
                    lit ? currentTheme->waterfallHighlight : currentTheme->waterfallDestroy, viewW, viewH,
                    hoverAlpha);
            }
            if (hoverDemod == nullptr) {
                long long bw = selectedDemodulator ? selectedDemodulator->getBandwidth()
                                                   : wxGetApp().getDemodMgr().getLastBandwidth();
                float ofs = (float)bw / (float)currentBandwidth;
                ImU32 selCol = ((paintIsNew && selectedDemodulator) || (!selectedDemodulator))
                    ? packColor(currentTheme->waterfallNew.r, currentTheme->waterfallNew.g,
                          currentTheme->waterfallNew.b, 0.51f)
                    : packColor(currentTheme->waterfallHover.r, currentTheme->waterfallHover.g,
                          currentTheme->waterfallHover.b, 0.51f);
                float cx = f.nx((mx01 - 0.5f) * 2.0f);
                f.dl->PushTexture(imguiAdditiveTexture());
                f.dl->AddLine(ImVec2(cx, f.ny(1.0f)), ImVec2(cx, f.ny(-1.0f)), selCol, 1.0f);
                if (last_type != "USB") {
                    f.dl->AddLine(ImVec2(f.nx((mx01 - 0.5f) * 2.0f - ofs), f.ny(1.0f)),
                        ImVec2(f.nx((mx01 - 0.5f) * 2.0f - ofs), f.ny(-1.0f)), selCol, 1.0f);
                }
                if (last_type != "LSB") {
                    f.dl->AddLine(ImVec2(f.nx((mx01 - 0.5f) * 2.0f + ofs), f.ny(1.0f)),
                        ImVec2(f.nx((mx01 - 0.5f) * 2.0f + ofs), f.ny(-1.0f)), selCol, 1.0f);
                }
                f.dl->PopTexture();
            } else if (hoverDemod != selectedDemodulator) {
                drawDemodEdges(f.dl, f.ox, f.oy, wsize.x, wsize.y, hoverDemod, currentTheme->waterfallHover,
                    viewW, viewH, hoverAlpha);
            }
        }
    } else {
        hoverAlpha += (0.0f - hoverAlpha) * 0.05f;
        if (hoverAlpha < 1.0e-5f) {
            hoverAlpha = 0.0f;
        }
        if (selectedDemodulator) {
            drawDemodEdges(f.dl, f.ox, f.oy, wsize.x, wsize.y, selectedDemodulator,
                currentTheme->waterfallHighlight, viewW, viewH, 0.0f);
        }
    }

    for (auto &loopDemod : demods) {
        if (!loopDemod->isActive() && loopDemod != selectedDemodulator) {
            continue;
        }
        if (hoverDemod == loopDemod || (selectedDemodulator == loopDemod && !dimSelection)) {
            continue;
        }
        drawDemodEdges(f.dl, f.ox, f.oy, wsize.x, wsize.y, loopDemod, currentTheme->waterfallHighlight, viewW,
            viewH, 0.0f);
    }

    for (auto &loopDemod : demods) {
        loopDemod->getVisualCue()->step();

        int squelchBreak = loopDemod->getVisualCue()->getSquelchBreak();
        if (squelchBreak) {
            drawDemodEdges(f.dl, f.ox, f.oy, wsize.x, wsize.y, loopDemod, currentTheme->waterfallHover, viewW,
                viewH, (float)squelchBreak / 60.0f);
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void WaterfallCanvas::drawDemodEdges(ImDrawList *dl, float ox, float oy, float w, float h,
    const DemodulatorInstancePtr &demod, const RGBA4f &color, int viewW, int viewH, float washAlpha) {
    if (!demod) {
        return;
    }
    SpecFrame f{dl, ox, oy, w, h};
    long long centerFreqHz = getCenterFrequency();
    long long spanHz = getBandwidth();
    long long demodFreq = demod->getFrequency();
    if (demod->isDeltaLock()) {
        demodFreq = wxGetApp().getFrequency() + demod->getDeltaLockOfs();
    }
    float uxUnit = (float)(demodFreq - (centerFreqHz - spanHz / 2)) / (float)spanHz;

    DemodMarkers::MarkerInput in;
    in.freqHz = demodFreq;
    in.bandwidthHz = demod->getBandwidth();
    in.sideband = DemodMarkers::sidebandForType(demod->getDemodulatorType());
    in.viewCenterHz = centerFreqHz;
    in.viewSpanHz = spanHz;
    in.viewWidthPx = viewW;
    in.viewHeightPx = viewH;
    DemodMarkers::MarkerGeometry geo = DemodMarkers::markerGeometry(in);

    ImU32 edgeCol = packColor(color.r, color.g, color.b, 0.51f);
    dl->PushTexture(imguiAdditiveTexture());
    float cx = f.nx(geo.uxPos);
    dl->AddLine(ImVec2(cx, f.ny(1.0f)), ImVec2(cx, f.ny(-1.0f)), edgeCol, 1.0f);
    float lx = f.nx(geo.uxPos - geo.ofsLeft);
    dl->AddLine(ImVec2(lx, f.ny(1.0f)), ImVec2(lx, f.ny(-1.0f)), edgeCol, 1.0f);
    float rx = f.nx(geo.uxPos + geo.ofsRight);
    dl->AddLine(ImVec2(rx, f.ny(1.0f)), ImVec2(rx, f.ny(-1.0f)), edgeCol, 1.0f);
    if (washAlpha > 0.0f) {
        ImU32 washCol = packColor(color.r, color.g, color.b, 0.2f * washAlpha);
        dl->AddRectFilled(ImVec2(f.nx(geo.uxPos - geo.ofsLeft), f.ny(1.0f)),
            ImVec2(f.nx(geo.uxPos + geo.ofsRight), f.ny(-1.0f)), washCol);
    }
    dl->PopTexture();

    double fontScale = GLFont::getScaleFactor();
    float labelFontPx = 16.0f * (float)fontScale;
    float xOfs = 2.0f / (float)viewW;
    float hPos = 20.0f / (float)viewH;
    wxString demodStr(DemodMarkers::demodTypeLabel(demod->getDemodulatorType(), demod->getDemodulatorLock() != 0));
    if (demodStr == "LSB") {
        uxUnit -= xOfs;
    } else if (demodStr == "USB") {
        uxUnit += xOfs;
    }
    float labelX = f.nx(2.0f * (uxUnit - 0.5f));
    if (!demod->getDemodulatorUserLabel().empty()) {
        float typeY = f.ny(-1.0f + hPos * 1.2f + hPos * 1.2f * (float)fontScale);
        float userY = f.ny(-1.0f + hPos * 1.2f);
        drawCenterWText(f, demodStr.ToStdWstring(), labelX + 1.0f, typeY + 1.0f, labelFontPx,
            IM_COL32(0, 0, 0, 255));
        drawCenterWText(
            f, demodStr.ToStdWstring(), labelX, typeY, labelFontPx, IM_COL32_WHITE);
        drawCenterWText(f, demod->getDemodulatorUserLabel(), labelX + 1.0f, userY + 1.0f, labelFontPx,
            IM_COL32(0, 0, 0, 255));
        drawCenterWText(f, demod->getDemodulatorUserLabel(), labelX, userY, labelFontPx, IM_COL32_WHITE);
    } else {
        float labelY = f.ny(-1.0f + hPos * 1.2f);
        drawCenterWText(f, demodStr.ToStdWstring(), labelX + 1.0f, labelY + 1.0f, labelFontPx,
            IM_COL32(0, 0, 0, 255));
        drawCenterWText(f, demodStr.ToStdWstring(), labelX, labelY, labelFontPx, IM_COL32_WHITE);
    }
}
