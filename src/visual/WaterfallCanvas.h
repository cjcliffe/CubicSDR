// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "wx/event.h"

#include "ui/imgui/BgfxCanvas.h"
#include "ui/imgui/imgui_bgfx.h"
#include "ColorTheme.h"
#include "DemodulatorMgr.h"
#include "SpectrumVisualProcessor.h"
#include "WaterfallPacer.h"
#include "WaterfallRing.h"

class SpectrumCanvas;

class WaterfallCanvas : public BgfxCanvas {
public:
    enum DragState {
        WF_DRAG_NONE, WF_DRAG_BANDWIDTH_LEFT, WF_DRAG_BANDWIDTH_RIGHT, WF_DRAG_FREQUENCY, WF_DRAG_RANGE
    };

    WaterfallCanvas(wxWindow *parent, const std::string &windowName = "Waterfall");
    ~WaterfallCanvas() override;

    void setup(unsigned int fft_size_in, int waterfall_lines_in);
    void setFFTSize(unsigned int fft_size_in);

    DragState getDragState();
    DragState getNextDragState();

    void attachSpectrumCanvas(SpectrumCanvas *canvas_in);
    void processInputQueue();
    SpectrumVisualDataQueuePtr getVisualDataQueue();

    // Demodulator currently under the cursor; never affects selection.
    DemodulatorInstancePtr getHoverDemod();
    void clearHoverDemod();

    void setLinesPerSecond(int lps);
    void setMinBandwidth(int min);

    void setView(long long center_freq_in, long long bandwidth_in);
    void disableView();
    bool getViewState() const;

    void setCenterFrequency(long long center_freq_in);
    long long getCenterFrequency() const;

    void setBandwidth(long long bandwidth_in);
    long long getBandwidth() const;

    // Public because AppFrame::OnGlobalKeyDown/OnGlobalKeyUp forward here.
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    // Public because called by SpectrumCanvas.
    void setMouseZoom(float mouseZoom_in);

    float hoverX01() const;
    bool isShiftHeld() const;

    void frameUpdate() override;

private:
    void drawView(const BgfxEdges &edges, float mx, float my) override;
    void onEnterView() override;
    void onLeaveView() override;

    long long getFrequencyAt(float x) const;
    long long getFrequencyAt(float x, long long iqCenterFreq, long long iqBandwidth) const;

    void updateHoverState(float mx01);
    void updateCenterFrequency(long long freq);
    void updateTextures();
    void drawDemodEdges(
        ImDrawList *dl, float ox, float oy, float w, float h, const DemodulatorInstancePtr &demod,
        const RGBA4f &color, int viewW, int viewH, float washAlpha);

    SpectrumCanvas *spectrumCanvas;
    std::string viewName;
    SpectrumVisualDataQueuePtr visualDataQueue = std::make_shared<SpectrumVisualDataQueue>();

    WaterfallRing ring;
    WaterfallPacer pacer;
    bgfx::TextureHandle tex[2];
    std::vector<unsigned char> shadow[2];
    std::vector<uint8_t> lut;
    std::vector<uint8_t> staging;
    ColorTheme *activeTheme;
    bool texReady;

    bool isView;
    long long centerFreq;
    long long bandwidth;
    long long lastBandwidth;

    DragState dragState;
    DragState nextDragState;

    DemodulatorInstancePtr hoverDemod;

    unsigned int fft_size, new_fft_size;
    int waterfall_lines;
    int dragOfs;

    float mouseZoom, zoom;
    bool freqMoving;
    long double freqMove;
    float hoverAlpha;
    float scaleMove;
    float rightDragDist;
    bool midZooming;
    int midZoomStartY;
    int dragBW;

    bool shiftDown, altDown, ctrlDown;
    bool leftDown, rightDown;
    float originX01, originY01;
    float lastX01, lastY01;
    float lastDragX01;
    float mouseX01;

    std::chrono::steady_clock::time_point lastFrameTime;
    std::mutex tex_update;
    int minBandwidth;
    std::atomic_bool fft_size_changed;
};
