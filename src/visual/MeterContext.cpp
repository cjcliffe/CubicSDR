#include "MeterContext.h"
#include "MeterCanvas.h"

MeterContext::MeterContext(MeterCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void MeterContext::DrawBegin() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
}

void MeterContext::Draw(float r, float g, float b, float a, float level) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(1.0, -1.0 + 2.0 * level);
    glVertex2f(-1.0, -1.0 + 2.0 * level);
    glVertex2f(-1.0, -1.0);
    glVertex2f(1.0, -1.0);
    glEnd();
    glDisable(GL_BLEND);
}

void MeterContext::DrawEnd() {
    glFlush();

    CheckGLError();
}

