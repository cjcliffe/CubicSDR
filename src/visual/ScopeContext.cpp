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

void ScopeContext::DrawBegin(bool clear) {
    if (clear) {
        glClearColor(ThemeMgr::mgr.currentTheme->scopeBackground.r, ThemeMgr::mgr.currentTheme->scopeBackground.g,
                ThemeMgr::mgr.currentTheme->scopeBackground.b, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();

    glDisable (GL_TEXTURE_2D);
}

void ScopeContext::DrawTunerTitles(bool ppmMode) {
    glLoadIdentity();

    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float viewHeight = (float) vp[3];
    float hPos = (float) (13) / viewHeight;

    glColor3f(0.65f, 0.65f, 0.65f);

    GLFont::Drawer refDrawingFont = GLFont::getFont(12, GLFont::getScaleFactor());

    refDrawingFont.drawString(ppmMode?"Device PPM":"Frequency", -0.66f, -1.0+hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    refDrawingFont.drawString("Bandwidth", 0.0, -1.0+hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    refDrawingFont.drawString("Center Frequency", 0.66f, -1.0+hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
}

void ScopeContext::DrawDeviceName(std::string deviceName) {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float viewHeight = (float) vp[3];
    float hPos = (float) (viewHeight - 20) / viewHeight;

    glColor3f(0.65f, 0.65f, 0.65f);

    GLFont::getFont(12, GLFont::getScaleFactor()).drawString(deviceName.c_str(), 1.0, hPos, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
}

void ScopeContext::DrawEnd() {
//    glFlush();

//    CheckGLError();
}

void ScopeContext::DrawDivider() {
    glColor3f(1.0, 1.0, 1.0);
    glBegin (GL_LINES);
    glVertex2f(0.0, -1.0);
    glVertex2f(0.0, 1.0);
    glEnd();
}
