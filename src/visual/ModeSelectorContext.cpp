// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModeSelectorContext.h"
#include "ModeSelectorCanvas.h"
#include "ColorTheme.h"
#include "ModeSelector.h"


ModeSelectorContext::ModeSelectorContext(ModeSelectorCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs *ctxAttrs) :
        PrimaryGLContext(canvas, sharedContext, ctxAttrs) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void ModeSelectorContext::DrawBegin() {
    glClearColor(ThemeMgr::mgr.currentTheme->generalBackground.r, ThemeMgr::mgr.currentTheme->generalBackground.g, ThemeMgr::mgr.currentTheme->generalBackground.b,1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
}

void ModeSelectorContext::DrawSelector(const std::string& label, int c, int cMax, bool on, float r, float g, float b, float a, float px, float py) {
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];
    float viewWidth = (float) vp[2];

    ModeSelector::SelectorCell cell = ModeSelector::selectorCell(c, cMax, viewWidth, viewHeight, px, py);

    glColor4f(r, g, b, a);

    if (a < 1.0) {
        glEnable(GL_BLEND);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &cell.outline[0]);
    glDrawArrays(on?GL_QUADS:GL_LINE_LOOP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    if (a < 1.0) {
        glDisable(GL_BLEND);
    }

    if (on) {
        glColor4f(0, 0, 0, a);
    }

    //Do not zoom the selectors
    GLFont::getFont(cell.fontSize).drawString(label, 0.0, cell.labelY, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
}

void ModeSelectorContext::DrawEnd() {
//    glFlush();

//    CheckGLError();
}

