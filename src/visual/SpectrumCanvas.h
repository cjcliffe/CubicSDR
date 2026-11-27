// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "wx/event.h"

#include "ui/imgui/BgfxCanvas.h"
#include "SpectrumVisualProcessor.h"

class WaterfallCanvas;

class SpectrumCanvas : public BgfxCanvas {
public:
    SpectrumCanvas(wxWindow *parent, const std::string &windowName = "Spectrum");
    ~SpectrumCanvas() override;

    // Public because AppFrame::OnGlobalKeyDown/OnGlobalKeyUp forward here.
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void attachWaterfallCanvas(WaterfallCanvas *canvas_in);
    void moveCenterFrequency(long long freqChange);

    void setShowDb(bool showDb);
    bool getShowDb() const;

    void setUseDBOfs(bool useDbOfs);
    bool getUseDBOfs() const;

    void setView(long long center_freq_in, long long bandwidth_in);
    void disableView();
    bool getViewState() const;

    void setCenterFrequency(long long center_freq_in);
    long long getCenterFrequency() const;

    void setBandwidth(long long bandwidth_in);
    long long getBandwidth() const;

    void setScaleFactorEnabled(bool en);
    void setFFTSize(int fftSize);

    float hoverX01() const;

    SpectrumVisualDataQueuePtr getVisualDataQueue();

    // Called by Waterfall to forward the update of the vertical scale.
    void updateScaleFactorFromYMove(float yDeltaMouseMove);
    // Called by Waterfall to reset the shared scale factor on right-click.
    void resetScaleFactorFromClick();

    void frameUpdate() override;

private:
    void drawView(const BgfxEdges &edges, float mx, float my) override;
    void onEnterView() override;
    void onLeaveView() override;

    long long getFrequencyAt(float x) const;
    void updateScaleFactor(float factor);
    void updateCursor(bool left, bool right);

    WaterfallCanvas *waterfallCanvas;
    std::string viewName;
    SpectrumVisualDataQueuePtr visualDataQueue = std::make_shared<SpectrumVisualDataQueue>();

    std::vector<float> points;
    std::vector<float> peakPoints;
    float floorValue, ceilValue;
    int fftSize;
    bool showDb, useDbOfs;

    bool isView;
    long long centerFreq;
    long long bandwidth;
    long long lastBandwidth;

    float scaleFactor;
    int bwChange;
    bool resetScaleFactor, scaleFactorEnabled;

    bool shiftDown, altDown, ctrlDown;
    bool leftDown, rightDown;
    float lastPanX01, lastGainY01, rightOriginY01;
    float mouseX01;
    float lastStatusX, lastStatusY;
    int lastCursor;
};
