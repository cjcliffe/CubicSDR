#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"
#include "ColorTheme.h"

class WaterfallCanvas;

class WaterfallContext: public PrimaryGLContext {
public:
    WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext);

    void Draw(std::vector<float> &points, bool step);
    void Setup(int fft_size_in, int num_waterfall_lines_in);
    void refreshTheme();

private:
    GLuint waterfall[2];
    int waterfall_ofs[2];
    int fft_size;
    int waterfall_lines;
	unsigned char *waterfall_slice;

    ColorTheme *activeTheme;
};
