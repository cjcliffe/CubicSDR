// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "GLPanel.h"
#include "WaterfallRing.h"
#include <atomic>

class WaterfallPanel : public GLPanel {
public:
    WaterfallPanel();
    void setup(unsigned int fft_size_in, int num_waterfall_lines_in);
    void refreshTheme();
    void setPoints(std::vector<float> &points_in);
    void step();
    void update();
    
protected:
    void drawPanelContents() override;
    
private:
    GLuint waterfall[2];
    unsigned int fft_size;
    int waterfall_lines;
    std::atomic_bool texInitialized;

    WaterfallRing ring;

    ColorTheme *activeTheme;
};
