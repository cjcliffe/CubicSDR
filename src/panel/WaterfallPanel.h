#pragma once

#include "GLPanel.h"

class WaterfallPanel : public GLPanel {
public:
    WaterfallPanel();
    void setup(int fft_size_in, int num_waterfall_lines_in);
    void refreshTheme();
    void setPoints(std::vector<float> &points);
    void step();
    void update();
    
protected:
    void drawPanelContents();
    
private:
    std::vector<float> points;

    GLuint waterfall[2];
    int waterfall_ofs[2];
    int fft_size;
    int waterfall_lines;
    unsigned char *waterfall_slice;
    std::vector<unsigned char> lineBuffer[2];
    std::vector<unsigned char> rLineBuffer[2];
    int lines_buffered;
    
    ColorTheme *activeTheme;
};
