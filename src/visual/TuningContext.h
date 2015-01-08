#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

#define NUM_WATERFALL_LINES 512

class TuningCanvas;

class TuningContext: public PrimaryGLContext {
public:
    TuningContext(TuningCanvas *canvas, wxGLContext *sharedContext);

    void DrawBegin();
    void Draw(float r, float g, float b, float a, float p1, float p2);
    void DrawDemodFreqBw(long long freq, unsigned int bw, long long center);
    void DrawEnd();

private:
    std::locale comma_locale;
    std::stringstream freqStr;
};
