#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"
#include "DemodulatorMgr.h"

#define NUM_WATERFALL_LINES 512

class WaterfallCanvas;

class WaterfallContext: public PrimaryGLContext {
public:
    WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext);

    void BeginDraw();
    void Draw(std::vector<float> &points);
    void DrawFreqSelector(float uxPos, float r = 1, float g = 1, float b = 1);
    void DrawDemod(DemodulatorInstance *demod, float r = 1, float g = 1, float b = 1);
    void EndDraw();

private:
    Gradient grad;
    GLuint waterfall;
    unsigned char waterfall_tex[FFT_SIZE * NUM_WATERFALL_LINES];
};
