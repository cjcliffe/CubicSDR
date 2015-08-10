#pragma once

#include "GLPanel.h"

class WaterfallPanel : public GLPanel {
public:
    WaterfallPanel();
    void setup(int fft_size_in, int num_waterfall_lines_in);
    void refreshTheme();
    void setPoints(std::vector<float> &points);
    void step();
    
    std::vector<float> points;
    bool needsUpdate;

protected:
    void drawPanelContents();
    
private:
    GLuint waterfall[2];
    int waterfall_ofs[2];
    int fft_size;
    int waterfall_lines;
    unsigned char *waterfall_slice;
    
    ColorTheme *activeTheme;
};