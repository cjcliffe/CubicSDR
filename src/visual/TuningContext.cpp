// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "TuningContext.h"
#include "TuningCanvas.h"

#include "ColorTheme.h"
#include "TunerLayout.h"
#include <vector>

// http://stackoverflow.com/questions/7276826/c-format-number-with-commas
class comma_numpunct: public std::numpunct<char> {
protected:
    char do_thousands_sep() const override {
        return ',';
    }

    std::string do_grouping() const override {
        return "\03";
    }
};

TuningContext::TuningContext(TuningCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs *ctxAttrs) :
        PrimaryGLContext(canvas, sharedContext, ctxAttrs) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    comma_locale = std::locale(std::locale(), new comma_numpunct());
    freqStrFormatted.imbue(comma_locale);
}

void TuningContext::DrawBegin() {
    glClearColor(ThemeMgr::mgr.currentTheme->generalBackground.r, ThemeMgr::mgr.currentTheme->generalBackground.g,
            ThemeMgr::mgr.currentTheme->generalBackground.b, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
}

void TuningContext::Draw(float r, float g, float b, float a, float p1, float p2) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    std::vector<float> barVerts = TunerLayout::dragBarQuads(p1, p2);
    float barColors[32] = {
        r * 0.5f, g * 0.5f, b * 0.5f, a, r * 0.5f, g * 0.5f, b * 0.5f, a,
        r, g, b, a, r, g, b, a,
        r, g, b, a, r, g, b, a,
        r * 0.5f, g * 0.5f, b * 0.5f, a, r * 0.5f, g * 0.5f, b * 0.5f, a
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &barVerts[0]);
    glColorPointer(4, GL_FLOAT, 0, barColors);
    glDrawArrays(GL_QUADS, 0, 8);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
}

void TuningContext::DrawEnd() {
//    glFlush();

//    CheckGLError();
}

void TuningContext::DrawTuner(long long freq, int count, float displayPos, float displayWidth) {
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewWidth = (float) vp[2];
    float viewHeight = (float) vp[3];

    freqStr.str("");
    freqStr << freq;
    std::string freqChars = freqStr.str();

    int fontSize = TunerLayout::tunerFontSize(viewWidth, viewHeight);

    glColor3f(ThemeMgr::mgr.currentTheme->text.r, ThemeMgr::mgr.currentTheme->text.g, ThemeMgr::mgr.currentTheme->text.b);
    int numChars = freqChars.length();
    std::vector<TunerLayout::TunerChar> chars = TunerLayout::tunerChars(count, numChars, displayPos, displayWidth);

    //do not zoom this one:
    GLFont::Drawer refDrawingFont = GLFont::getFont(fontSize);

    for (auto &ch : chars) {
        refDrawingFont.drawString(freqStr.str().substr(ch.charIndex, 1), ch.x, 0, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
    }

    glColor4f(0.65f, 0.65f, 0.65f, 0.25f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    std::vector<float> gridVerts = TunerLayout::tunerGridVerts(count, displayPos, displayWidth);
    if (!gridVerts.empty()) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &gridVerts[0]);
        glDrawArrays(GL_LINES, 0, gridVerts.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    glDisable(GL_BLEND);
}


void TuningContext::DrawTunerDigitBox(int index, int count, float displayPos, float displayWidth, const RGBA4f& /* c */) {
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];
    std::vector<float> boxVerts = TunerLayout::tunerDigitBox(index, count, displayPos, displayWidth, viewHeight);
    
    glColor4f(1.0, 0,0,1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &boxVerts[0]);
    glDrawArrays(GL_LINE_STRIP, 0, 5);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}


int TuningContext::GetTunerDigitIndex(float mPos, int count, float displayPos, float displayWidth) {
    return TunerLayout::digitIndexAt(mPos, count, displayPos, displayWidth);
}

void TuningContext::DrawTunerBarIndexed(int start, int end, int count, float displayPos, float displayWidth, const RGBA4f& color, float /* alpha */, bool top,
bool bottom) {
    TunerLayout::TunerBars bars = TunerLayout::tunerBarQuads(start, end, count, displayPos, displayWidth, top, bottom);
    
    float r = color.r, g = color.g, b = color.b, a = 0.6f;
    
    float topColors[16] = { r * 0.5f, g * 0.5f, b * 0.5f, a, r * 0.5f, g * 0.5f, b * 0.5f, a, r, g, b, a, r, g, b, a };
    float bottomColors[16] = { r, g, b, a, r, g, b, a, r * 0.5f, g * 0.5f, b * 0.5f, a, r * 0.5f, g * 0.5f, b * 0.5f, a };
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    if (!bars.top.empty()) {
        glVertexPointer(2, GL_FLOAT, 0, &bars.top[0]);
        glColorPointer(4, GL_FLOAT, 0, topColors);
        glDrawArrays(GL_QUADS, 0, 4);
    }
    if (!bars.bottom.empty()) {
        glVertexPointer(2, GL_FLOAT, 0, &bars.bottom[0]);
        glColorPointer(4, GL_FLOAT, 0, bottomColors);
        glDrawArrays(GL_QUADS, 0, 4);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_BLEND);
}

void TuningContext::DrawDemodFreqBw(long long freq, unsigned int bw, long long center) {
    DrawTuner(freq, 11, -1.0, (1.0f / 3.0f) * 2.0f);
    DrawTuner(bw, 7, -1.0 + (2.25f / 3.0f), (1.0f / 4.0f) * 2.0f);
    DrawTuner(center, 11, -1.0 + (2.0f / 3.0f) * 2.0, (1.0f / 3.0f) * 2.0f);
}

