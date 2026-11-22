// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "MeterContext.h"
#include "MeterCanvas.h"
#include "ColorTheme.h"

MeterContext::MeterContext(MeterCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs *ctxAttrs) :
        PrimaryGLContext(canvas, sharedContext, ctxAttrs) {
}

void MeterContext::DrawBegin() {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glClearColor(ThemeMgr::mgr.currentTheme->generalBackground.r, ThemeMgr::mgr.currentTheme->generalBackground.g, ThemeMgr::mgr.currentTheme->generalBackground.b, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
}

void MeterContext::Draw(float r, float g, float b, float a, float level) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    float meterVerts[16] = {
        -1.0, -1.0 + 2.0 * level, -1.0, -1.0,
        0.0, -1.0, 0.0, -1.0 + 2.0 * level,
        0.0, -1.0 + 2.0 * level, 0.0, -1.0,
        1.0, -1.0, 1.0, -1.0 + 2.0 * level
    };
    float meterColors[32] = {
        r * 0.65, g * 0.65, b * 0.65, a, r * 0.65, g * 0.65, b * 0.65, a,
        r, g, b, a, r, g, b, a,
        r, g, b, a, r, g, b, a,
        r * 0.65, g * 0.65, b * 0.65, a * 0.65, r * 0.65, g * 0.65, b * 0.65, a * 0.65
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, meterVerts);
    glColorPointer(4, GL_FLOAT, 0, meterColors);
    glDrawArrays(GL_QUADS, 0, 8);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
}

void MeterContext::DrawEnd() {
//    glFlush();

//    CheckGLError();
}

