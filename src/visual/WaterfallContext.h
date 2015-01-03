#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

class WaterfallCanvas;

class WaterfallContext: public PrimaryGLContext {
public:
    WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext);

    void Draw(std::vector<float> &points);
    void Setup(int fft_size_in, int num_waterfall_lines_in);

private:
    Gradient grad;
    GLuint waterfall;
    unsigned char *waterfall_tex;
    int fft_size;
    int waterfall_lines;
};
