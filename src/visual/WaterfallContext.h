#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"


#define COLOR_THEME_DEFAULT 0
#define COLOR_THEME_BW 1
#define COLOR_THEME_SHARP 2
#define COLOR_THEME_RAD 3
#define COLOR_THEME_TOUCH 4
#define COLOR_THEME_HD 5
#define COLOR_THEME_RADAR 6
#define COLOR_THEME_MAX 7

class WaterfallCanvas;

class WaterfallContext: public PrimaryGLContext {
public:
    WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext);

    void Draw(std::vector<float> &points);
    void Setup(int fft_size_in, int num_waterfall_lines_in);
    void setTheme(int theme_id);
    int getTheme();

private:
    Gradient *gradients[COLOR_THEME_MAX];
    GLuint waterfall;
    int theme;
    unsigned char *waterfall_tex;
    int fft_size;
    int waterfall_lines;
};
