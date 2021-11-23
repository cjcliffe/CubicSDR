// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "GLPanel.h"
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
    std::vector<float> points;

    GLuint waterfall[2];
    int waterfall_ofs[2];
    unsigned int fft_size;
    int waterfall_lines;
    unsigned char *waterfall_slice;
    std::vector<unsigned char> lineBuffer[2];
    std::vector<unsigned char> rLineBuffer[2];
    std::atomic_int lines_buffered;
    std::atomic_bool texInitialized, bufferInitialized;
    
    ColorTheme *activeTheme;
};
