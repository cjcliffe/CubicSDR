// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

class MeterCanvas;

class MeterContext: public PrimaryGLContext {
public:
    MeterContext(MeterCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs *ctxAttrs);

    void DrawBegin();
    void Draw(float r, float g, float b, float a, float level);
    void DrawEnd();

private:
};
