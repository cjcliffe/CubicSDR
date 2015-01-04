#include "SpectrumContext.h"

#include "SpectrumCanvas.h"
#include "CubicSDR.h"
#include <sstream>
#include <iostream>

SpectrumContext::SpectrumContext(SpectrumCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext), fft_size(0) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

}

void SpectrumContext::Draw(std::vector<float> &points, long long freq, int bandwidth) {

    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0, 1.0, 1.0);

    if (points.size()) {
        glPushMatrix();
        glTranslatef(-1.0f, -0.75f, 0.0f);
        glScalef(2.0f, 1.5f, 1.0f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
    }

    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];
    float viewWidth = (float) vp[2];

    long long leftFreq = (float) freq - ((float) bandwidth / 2.0);
    long long rightFreq = leftFreq + (float) bandwidth;

    long long firstMhz = (leftFreq / 1000000) * 1000000;
    long double mhzStart = ((long double)(firstMhz - leftFreq) / (long double)(rightFreq - leftFreq)) * 2.0;
    long double mhzStep = (100000.0 / (long double)(rightFreq - leftFreq)) * 2.0;

    long double currentMhz = trunc(floor(firstMhz / 1000000.0));

    std::stringstream label;
    label.precision(2);

    float hPos = 1.0 - (16.0 / viewHeight);
    float lMhzPos = 1.0 - (5.0 / viewHeight);

    for (float m = -1.0 + mhzStart, mMax = 1.0 + fabs(mhzStart); m <= mMax; m += mhzStep) {
        label << std::fixed << currentMhz;

        double fractpart, intpart;

        fractpart = modf(currentMhz, &intpart);

        if (fractpart < 0.001) {
            glLineWidth(4.0);
            glColor3f(1.0, 1.0, 1.0);
        } else {
            glLineWidth(1.0);
            glColor3f(0.55, 0.55, 0.55);
        }

        glDisable(GL_TEXTURE_2D);
        glBegin(GL_LINES);
        glVertex2f(m, lMhzPos);
        glVertex2f(m, 1);
        glEnd();

        getFont(PrimaryGLContext::GLFONT_SIZE12).drawString(label.str(), m, hPos, 12, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);

        label.str(std::string());

        currentMhz += 0.1f;
    }

    glLineWidth(1.0);

//    getFont(PrimaryGLContext::GLFONT_SIZE16).drawString("Welcome to CubicSDR -- This is a test string. 01234567890!@#$%^&*()_[]",0.0,0.0,16,GLFont::GLFONT_ALIGN_CENTER,GLFont::GLFONT_ALIGN_CENTER);
    CheckGLError();
}
