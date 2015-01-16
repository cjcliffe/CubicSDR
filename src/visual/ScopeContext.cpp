#include "ScopeContext.h"

#include "ScopeCanvas.h"
#include "ColorTheme.h"

ScopeContext::ScopeContext(ScopeCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
    glDisable (GL_CULL_FACE);
    glDisable (GL_DEPTH_TEST);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
}

void ScopeContext::DrawBegin() {
    glClearColor(ThemeMgr::mgr.currentTheme->scopeBackground.r, ThemeMgr::mgr.currentTheme->scopeBackground.g,
            ThemeMgr::mgr.currentTheme->scopeBackground.b, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();

    glDisable (GL_TEXTURE_2D);
}

void ScopeContext::Plot(std::vector<float> &points, bool stereo) {
    if (stereo) {
        glBegin(GL_QUADS);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r, ThemeMgr::mgr.currentTheme->scopeBackground.g, ThemeMgr::mgr.currentTheme->scopeBackground.b);
        glVertex2f(1, 1);
        glVertex2f(-1, 1);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r*2.0, ThemeMgr::mgr.currentTheme->scopeBackground.g*2.0, ThemeMgr::mgr.currentTheme->scopeBackground.b*2.0);
        glVertex2f(-1, 0.5);
        glVertex2f(1, 0.5);

        glVertex2f(-1, 0.5);
        glVertex2f(1, 0.5);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r, ThemeMgr::mgr.currentTheme->scopeBackground.g, ThemeMgr::mgr.currentTheme->scopeBackground.b);
        glVertex2f(1, 0.0);
        glVertex2f(-1, 0.0);

        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r, ThemeMgr::mgr.currentTheme->scopeBackground.g, ThemeMgr::mgr.currentTheme->scopeBackground.b);
        glVertex2f(1, 0);
        glVertex2f(-1, 0);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r*2.0, ThemeMgr::mgr.currentTheme->scopeBackground.g*2.0, ThemeMgr::mgr.currentTheme->scopeBackground.b*2.0);
        glVertex2f(-1, -0.5);
        glVertex2f(1, -0.5);

        glVertex2f(-1, -0.5);
        glVertex2f(1, -0.5);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r, ThemeMgr::mgr.currentTheme->scopeBackground.g, ThemeMgr::mgr.currentTheme->scopeBackground.b);
        glVertex2f(1, -1.0);
        glVertex2f(-1, -1.0);
        glEnd();
    } else {
        glBegin (GL_QUADS);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r, ThemeMgr::mgr.currentTheme->scopeBackground.g,
                ThemeMgr::mgr.currentTheme->scopeBackground.b);
        glVertex2f(1, 1);
        glVertex2f(-1, 1);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r * 2.0, ThemeMgr::mgr.currentTheme->scopeBackground.g * 2.0,
                ThemeMgr::mgr.currentTheme->scopeBackground.b * 2.0);
        glVertex2f(-1, 0);
        glVertex2f(1, 0);

        glVertex2f(-1, 0);
        glVertex2f(1, 0);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeBackground.r, ThemeMgr::mgr.currentTheme->scopeBackground.g,
                ThemeMgr::mgr.currentTheme->scopeBackground.b);
        glVertex2f(1, -1);
        glVertex2f(-1, -1);
        glEnd();
    }
    glLineWidth(1.0);

    if (stereo) {
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r, ThemeMgr::mgr.currentTheme->scopeLine.g, ThemeMgr::mgr.currentTheme->scopeLine.b);
        glBegin (GL_LINES);
        glVertex2f(-1.0, 0.0);
        glVertex2f(1.0, 0.0);
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r * 0.35, ThemeMgr::mgr.currentTheme->scopeLine.g * 0.35,
                ThemeMgr::mgr.currentTheme->scopeLine.b * 0.35);
        glVertex2f(-1.0, 0.5);
        glVertex2f(1.0, 0.5);
        glVertex2f(-1.0, -0.5);
        glVertex2f(1.0, -0.5);
        glEnd();
    } else {
        glColor3f(ThemeMgr::mgr.currentTheme->scopeLine.r * 0.35, ThemeMgr::mgr.currentTheme->scopeLine.g * 0.35,
                ThemeMgr::mgr.currentTheme->scopeLine.b * 0.35);
        glBegin (GL_LINES);
        glVertex2f(-1.0, 0.0);
        glVertex2f(1.0, 0.0);
        glEnd();
    }

    if (points.size()) {
        glEnable (GL_BLEND);
        glEnable (GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(ThemeMgr::mgr.currentTheme->scopeLine.r, ThemeMgr::mgr.currentTheme->scopeLine.g, ThemeMgr::mgr.currentTheme->scopeLine.b, 1.0);
        glEnableClientState (GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        if (stereo) {
            glLineWidth(1.5);
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
            glLineWidth(1.5);
            glPushMatrix();
            glTranslatef(-1.0f, 0.0f, 0.0f);
            glScalef(2.0f, 2.0f, 1.0f);
            glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
            glPopMatrix();
        }
        glLineWidth(1.0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisable(GL_BLEND);
    }
}

void ScopeContext::DrawDeviceName(std::string deviceName) {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float viewHeight = (float) vp[3];
    float hPos = (float) (viewHeight - 20) / viewHeight;

    glColor3f(0.65, 0.65, 0.65);
    getFont(PrimaryGLContext::GLFONT_SIZE12).drawString(deviceName.c_str(), 1.0, hPos, 12, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
}

void ScopeContext::DrawEnd() {
    glFlush();

    CheckGLError();
}

void ScopeContext::DrawDivider() {
    glColor3f(1.0, 1.0, 1.0);
    glBegin (GL_LINES);
    glVertex2f(0.0, -1.0);
    glVertex2f(0.0, 1.0);
    glEnd();
}
