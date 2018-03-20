// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

#define NUM_WATERFALL_LINES 512

class ScopeCanvas;

class ScopeContext: public PrimaryGLContext {
public:
    ScopeContext(ScopeCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs *ctxAttrs);

    void DrawBegin(bool clear=true);
    void DrawTunerTitles(bool ppmMode=false);
    void DrawDeviceName(std::string deviceName);
    void DrawDivider();
    void DrawEnd();

private:
};
