#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

#define NUM_WATERFALL_LINES 512

class ScopeCanvas;

class ScopeContext: public PrimaryGLContext {
public:
    ScopeContext(ScopeCanvas *canvas, wxGLContext *sharedContext);

    void DrawBegin();
    void Plot(std::vector<float> &points);
    void DrawDivider();
    void DrawEnd();

private:
};
