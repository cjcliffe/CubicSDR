#include "ModeSelectorContext.h"
#include "ModeSelectorCanvas.h"

ModeSelectorContext::ModeSelectorContext(ModeSelectorCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void ModeSelectorContext::DrawBegin() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
}

void ModeSelectorContext::DrawSelector(std::string label, int c, int cMax, bool on, float r, float g, float b, float a) {
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];
    float viewWidth = (float) vp[2];

    PrimaryGLContext::GLFontSize fontSize = GLFONT_SIZE16;

    int fontHeight = 16;

    if (viewWidth < 30) {
        fontSize = GLFONT_SIZE12;
        fontHeight = 12;
    }

    glColor4f(r, g, b, a);

    float y = 1.0 - ((float) c / (float) cMax * 2.0);
    float height = (2.0 / (float) cMax);
    float padX = (4.0 / viewWidth);
    float padY = (4.0 / viewHeight);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-1.0 + padX, y + padY);
    glVertex2f(1.0 - padX, y + padY);
    glVertex2f(1.0 - padX, y + height - padY);
    glVertex2f(-1.0 + padX, y + height - padY);
    glEnd();

    getFont(fontSize).drawString(label, 0.0, y + height / 2.0, fontHeight, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);

//    glEnable(GL_BLEND);
//    glBlendFunc(GL_ONE, GL_ONE);
//    glColor4f(r, g, b, a);

}

void ModeSelectorContext::DrawEnd() {
    glFlush();

    CheckGLError();
}

