// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "PrimaryGLContext.h"
#include "GLPanel.h"
#include "MeterPanel.h"

class UITestCanvas;

class UITestContext: public PrimaryGLContext {
public:
    UITestContext(UITestCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs *ctxAttrs);
    
    void DrawBegin();
    void Draw();
    void DrawEnd();
    
private:
    GLPanel testPanel;
    GLTestPanel testChildPanel;
    GLPanel testChildPanel2;
    GLPanel testChildPanel3;
    GLTextPanel testText1;
    MeterPanel testMeter;
};
