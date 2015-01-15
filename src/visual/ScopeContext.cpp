#include "ScopeContext.h"

#include "ScopeCanvas.h"
#include "ColorTheme.h"

ScopeContext::ScopeContext(ScopeCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void ScopeContext::DrawBegin() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
}

void ScopeContext::Plot(std::vector<float> &points, bool stereo) {
    glColor3f(1.0, 1.0, 1.0);

    if (stereo) {
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r, ThemeMgr::mgr.currentTheme->scopeLine.g, ThemeMgr::mgr.currentTheme->scopeLine.b);
        glBegin(GL_LINES);
        glVertex2f(-1.0, 0.0);
        glVertex2f(1.0, 0.0);
        glEnd();
        glColor3f(0.3, 0.3, 0.3);
        glBegin(GL_LINES);
        glVertex2f(-1.0, 0.5);
        glVertex2f(1.0, 0.5);
        glVertex2f(-1.0, -0.5);
        glVertex2f(1.0, -0.5);
        glEnd();
    } else {
        glColor3f(0.3, 0.3, 0.3);
        glBegin(GL_LINES);
        glVertex2f(-1.0, 0.0);
        glVertex2f(1.0, 0.0);
        glEnd();
    }

    glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r, ThemeMgr::mgr.currentTheme->scopeLine.g, ThemeMgr::mgr.currentTheme->scopeLine.b);
    if (points.size()) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        if (stereo) {
            glPushMatrix();
            glTranslatef(-1.0f, 0.5f, 0.0f);
            glScalef(4.0f, 0.92f, 1.0f);
            glDrawArrays(GL_LINE_STRIP, 0, points.size() / 4);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(-3.0f, -0.5f, 0.0f);
            glPushMatrix();
            glScalef(4.0f, 0.92f, 1.0f);
            glDrawArrays(GL_LINE_STRIP, points.size() / 4, points.size() / 4);
            glPopMatrix();
            glPopMatrix();
        } else {
            glPushMatrix();
            glTranslatef(-1.0f, 0.0f, 0.0f);
            glScalef(2.0f, 2.0f, 1.0f);
            glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
            glPopMatrix();
        }
        glDisableClientState(GL_VERTEX_ARRAY);

    }
}

void ScopeContext::DrawDeviceName(std::string deviceName) {
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);
    float viewHeight = (float) vp[3];
    float hPos = (float) (viewHeight - 20) / viewHeight;

    glColor3f(0.65,0.65,0.65);
    getFont(PrimaryGLContext::GLFONT_SIZE12).drawString(deviceName.c_str(), 1.0, hPos, 12, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
}

void ScopeContext::DrawEnd() {
    glFlush();

    CheckGLError();
}

void ScopeContext::DrawDivider() {
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex2f(0.0, -1.0);
    glVertex2f(0.0, 1.0);
    glEnd();
}
