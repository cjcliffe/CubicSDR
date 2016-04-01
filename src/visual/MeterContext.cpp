#include "MeterContext.h"
#include "MeterCanvas.h"
#include "ColorTheme.h"

MeterContext::MeterContext(MeterCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
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
    glBegin(GL_QUADS);
    glColor4f(r*0.65,g*0.65,b*0.65,a);
    glVertex2f(-1.0, -1.0 + 2.0 * level);
    glVertex2f(-1.0, -1.0);

    glColor4f(r,g,b,a);
    glVertex2f(0.0, -1.0);
    glVertex2f(0.0, -1.0 + 2.0 * level);

    glColor4f(r,g,b,a);
    glVertex2f(0.0, -1.0 + 2.0 * level);
    glVertex2f(0.0, -1.0);

    glColor4f(r*0.65,g*0.65,b*0.65,a*0.65);
    glVertex2f(1.0, -1.0);
    glVertex2f(1.0, -1.0 + 2.0 * level);

    glEnd();
    glDisable(GL_BLEND);
}

void MeterContext::DrawEnd() {
//    glFlush();

//    CheckGLError();
}

