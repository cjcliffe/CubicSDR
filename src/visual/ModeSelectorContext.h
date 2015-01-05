#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

#define NUM_WATERFALL_LINES 512

class ModeSelectorCanvas;

class ModeSelectorContext: public PrimaryGLContext {
public:
    ModeSelectorContext(ModeSelectorCanvas *canvas, wxGLContext *sharedContext);

    void DrawBegin();
    void Draw(float r, float g, float b, float a, float level);
    void DrawEnd();
};
