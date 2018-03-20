// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

#define NUM_WATERFALL_LINES 512

class ModeSelectorCanvas;

class ModeSelectorContext: public PrimaryGLContext {
public:
    ModeSelectorContext(ModeSelectorCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs *ctxAttrs);

    void DrawBegin();
    void DrawSelector(std::string label, int c, int cMax, bool on, float r, float g, float b, float a, float padx, float pady);
    void DrawEnd();
};
