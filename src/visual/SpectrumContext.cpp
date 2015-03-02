#include "SpectrumContext.h"

#include "SpectrumCanvas.h"
#include "CubicSDR.h"
#include <sstream>
#include <iostream>
#include "ColorTheme.h"

SpectrumContext::SpectrumContext(SpectrumCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext), fft_size(0), floorValue(0), ceilValue(1) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

}


float SpectrumContext::getFloorValue() const {
    return floorValue;
}

void SpectrumContext::setFloorValue(float floorValue) {
    this->floorValue = floorValue;
}

float SpectrumContext::getCeilValue() const {
    return ceilValue;
}

void SpectrumContext::setCeilValue(float ceilValue) {
    this->ceilValue = ceilValue;
}

void SpectrumContext::Draw(std::vector<float> &points, long long freq, int bandwidth) {

    glBegin(GL_QUADS);
    glColor3f(ThemeMgr::mgr.currentTheme->fftBackground.r, ThemeMgr::mgr.currentTheme->fftBackground.g, ThemeMgr::mgr.currentTheme->fftBackground.b);
    glVertex2f(1, 0.5);
    glVertex2f(-1, 0.5);
    glColor3f(ThemeMgr::mgr.currentTheme->fftBackground.r*2.0, ThemeMgr::mgr.currentTheme->fftBackground.g*2.0, ThemeMgr::mgr.currentTheme->fftBackground.b*2.0);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glEnd();


    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
    if (points.size()) {
        glPushMatrix();
        glTranslatef(-1.0f, -0.75f, 0.0f);
        glScalef(2.0f, 1.5f, 1.0f);


        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        float range = ceilValue-floorValue;
        float ranges[3][4] = { { 90.0, 5000.0, 10.0, 100.0 }, { 20.0, 150.0, 10.0, 10.0 }, { -20.0, 30.0, 10.0, 1.0 } };

        for (int i = 0; i < 3; i++) {
            float p = 0;
            float rangeMin = ranges[i][0];
            float rangeMax = ranges[i][1];
            float rangeTrans = ranges[i][2];
            float rangeStep = ranges[i][3];

            if (range >= rangeMin && range <= rangeMax) {
                float a = 1.0;

                if (range <= rangeMin+rangeTrans) {
                    a *= (range-rangeMin)/rangeTrans;
                }
                if (range >= rangeMax-rangeTrans) {
                    a *= (rangeTrans-(range-(rangeMax-rangeTrans)))/rangeTrans;
                }

                glColor4f(0.12, 0.12, 0.12, a);
                glBegin(GL_LINES);
                for (float l = floorValue; l<=ceilValue+rangeStep; l+=rangeStep) {
                    p += rangeStep/range;
                    glVertex2f(0,p);  glVertex2f(1,p);
                }
                glEnd();
            }
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor3f(ThemeMgr::mgr.currentTheme->fftLine.r, ThemeMgr::mgr.currentTheme->fftLine.g, ThemeMgr::mgr.currentTheme->fftLine.b);
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
    long double mhzStart = ((long double) (firstMhz - leftFreq) / (long double) (rightFreq - leftFreq)) * 2.0;
    long double mhzStep = (100000.0 / (long double) (rightFreq - leftFreq)) * 2.0;

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
            glColor3f(ThemeMgr::mgr.currentTheme->freqLine.r, ThemeMgr::mgr.currentTheme->freqLine.g, ThemeMgr::mgr.currentTheme->freqLine.b);
        } else {
            glLineWidth(1.0);
            glColor3f(ThemeMgr::mgr.currentTheme->freqLine.r * 0.65, ThemeMgr::mgr.currentTheme->freqLine.g * 0.65,
                    ThemeMgr::mgr.currentTheme->freqLine.b * 0.65);
        }

        glDisable(GL_TEXTURE_2D);
        glBegin(GL_LINES);
        glVertex2f(m, lMhzPos);
        glVertex2f(m, 1);
        glEnd();

        glColor4f(ThemeMgr::mgr.currentTheme->text.r, ThemeMgr::mgr.currentTheme->text.g, ThemeMgr::mgr.currentTheme->text.b,1.0);
        getFont(PrimaryGLContext::GLFONT_SIZE12).drawString(label.str(), m, hPos, 12, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);

        label.str(std::string());

        currentMhz += 0.1f;
    }

    glLineWidth(1.0);

//    getFont(PrimaryGLContext::GLFONT_SIZE16).drawString("Welcome to CubicSDR -- This is a test string. 01234567890!@#$%^&*()_[]",0.0,0.0,16,GLFont::GLFONT_ALIGN_CENTER,GLFont::GLFONT_ALIGN_CENTER);
    CheckGLError();
}
