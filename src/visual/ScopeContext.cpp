#include "ScopeContext.h"

#include "ScopeCanvas.h"

ScopeContext::ScopeContext(ScopeCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
    glDisable (GL_CULL_FACE);
    glDisable (GL_DEPTH_TEST);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
}

void ScopeContext::DrawBegin() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();

    glDisable (GL_TEXTURE_2D);
}

void ScopeContext::Plot(std::vector<float> &points) {
    glColor3f(1.0, 1.0, 1.0);

    if (points.size()) {
        glPushMatrix();
        glTranslatef(-1.0f, 0.0f, 0.0f);
        glScalef(2.0f, 2.0f, 1.0f);
        glEnableClientState (GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
    }
}

void ScopeContext::DrawEnd() {
    glFlush();

    CheckGLError();
}

void ScopeContext::DrawDivider() {
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex2f(0.0,-1.0);
    glVertex2f(0.0,1.0);
    glEnd();
}
