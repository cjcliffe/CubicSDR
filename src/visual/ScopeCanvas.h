// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "wx/event.h"

#include "ui/imgui/BgfxCanvas.h"
#include "ScopeCarousel.h"
#include "ScopeVisualProcessor.h"

class ScopeCanvas : public BgfxCanvas {
public:
    ScopeCanvas(wxWindow *parent);
    ~ScopeCanvas() override;

    // Public because AppFrame::OnGlobalKeyDown/OnGlobalKeyUp forward here.
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void setDeviceName(std::string device_name);
    void setPPMMode(bool ppmMode_in);
    bool getPPMMode() const;

    void setShowDb(bool showDb);
    bool getShowDb() const;

    bool scopeVisible();
    bool spectrumVisible();

    void setHelpTip(std::string tip);

    ScopeRenderDataQueuePtr getInputQueue();

    void frameUpdate() override;

private:
    void drawView(const BgfxEdges &edges, float mx, float my) override;
    void onEnterView() override;
    void onLeaveView() override;

    ScopeRenderDataQueuePtr inputData = std::make_shared<ScopeRenderDataQueue>();
    std::string deviceName;
    bool ppmMode;
    bool showDb;
    float panelSpacing;
    ScopeCarousel::CarouselState carousel;
    std::string helpTip;

    bool leftDown;
    float lastX01;

    int scopeMode;
    std::vector<float> wavePoints;
    std::vector<float> specPoints;
    double specFloor, specCeil;
    int specFftSize;
    long long specCenter, specSpan;
};
