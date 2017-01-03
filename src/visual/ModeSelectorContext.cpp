// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ModeSelectorContext.h"
#include "ModeSelectorCanvas.h"
#include "ColorTheme.h"


ModeSelectorContext::ModeSelectorContext(ModeSelectorCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
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

void ModeSelectorContext::DrawSelector(std::string label, int c, int cMax, bool on, float r, float g, float b, float a, float px, float py) {
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];
    float viewWidth = (float) vp[2];

    int fontSize = 18;

    if (viewWidth < 30 || viewHeight < 200) {
        fontSize = 16;
    }

    glColor4f(r, g, b, a);

    float y = 1.0 - ((float) (c+1) / (float) cMax * 2.0);
    float height = (2.0 / (float) cMax);
    float padX = (px / viewWidth);
    float padY = (py / viewHeight);

    if (a < 1.0) {
        glEnable(GL_BLEND);
    }
    glBegin(on?GL_QUADS:GL_LINE_LOOP);
    glVertex2f(-1.0 + padX, y + padY);
    glVertex2f(1.0 - padX, y + padY);
    glVertex2f(1.0 - padX, y + height - padY);
    glVertex2f(-1.0 + padX, y + height - padY);
    glEnd();
    if (a < 1.0) {
        glDisable(GL_BLEND);
    }

    if (on) {
        glColor4f(0, 0, 0, a);
    }

    //Do not zoom the selectors
    GLFont::getFont(fontSize).drawString(label, 0.0, y + height / 2.0, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
}

void ModeSelectorContext::DrawEnd() {
//    glFlush();

//    CheckGLError();
}

