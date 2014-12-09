#include "SpectrumContext.h"

#include "SpectrumCanvas.h"

SpectrumContext::SpectrumContext(SpectrumCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

}

void SpectrumContext::Draw(std::vector<float> &points) {

    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0, 1.0, 1.0);

    if (points.size()) {
        glPushMatrix();
        glTranslatef(-1.0f, -0.9f, 0.0f);
        glScalef(2.0f, 1.8f, 1.0f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
    }

    getFont()->drawString("Welcome to CubicSDR -- This is a test string. 01234567890!@#$%^&*()_[]",0.0,0.0,31,GLFont::GLFONT_ALIGN_CENTER,GLFont::GLFONT_ALIGN_CENTER);

    CheckGLError();
}
