#include "TuningContext.h"
#include "TuningCanvas.h"

#include "ColorTheme.h"

// http://stackoverflow.com/questions/7276826/c-format-number-with-commas
class comma_numpunct: public std::numpunct<char> {
protected:
    virtual char do_thousands_sep() const {
        return ',';
    }

    virtual std::string do_grouping() const {
        return "\03";
    }
};

TuningContext::TuningContext(TuningCanvas *canvas, wxGLContext *sharedContext) :
        PrimaryGLContext(canvas, sharedContext) {
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
    glBegin(GL_QUADS);
    glColor4f(r * 0.5, g * 0.5, b * 0.5, a);
    glVertex2f(-1.0 + p2 * 2.0, 1.0);
    glVertex2f(-1.0 + p1 * 2.0, 1.0);
    glColor4f(r, g, b, a);
    glVertex2f(-1.0 + p1 * 2.0, 0.0);
    glVertex2f(-1.0 + p2 * 2.0, 0.0);

    glVertex2f(-1.0 + p2 * 2.0, 0.0);
    glVertex2f(-1.0 + p1 * 2.0, 0.0);
    glColor4f(r * 0.5, g * 0.5, b * 0.5, a);
    glVertex2f(-1.0 + p1 * 2.0, -1.0);
    glVertex2f(-1.0 + p2 * 2.0, -1.0);
    glEnd();
    glDisable(GL_BLEND);
}

void TuningContext::DrawEnd() {
    glFlush();

    CheckGLError();
}

void TuningContext::DrawTuner(long long freq, int count, float displayPos, float displayWidth) {
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];

    freqStr.str("");
    freqStr << freq;
    std::string freqChars = freqStr.str();

    GLFont::GLFontSize fontSize = GLFont::GLFONT_SIZE24;
    int fontHeight = 24;

    if (viewHeight < 28) {
        fontSize = GLFont::GLFONT_SIZE18;
        fontHeight = 18;
    }
    if (viewHeight < 24) {
        fontSize = GLFont::GLFONT_SIZE16;
        fontHeight = 16;
    }
    if (viewHeight < 18) {
        fontSize = GLFont::GLFONT_SIZE12;
        fontHeight = 12;
    }

    glColor3f(ThemeMgr::mgr.currentTheme->text.r, ThemeMgr::mgr.currentTheme->text.g, ThemeMgr::mgr.currentTheme->text.b);
    int numChars = freqChars.length();
    int ofs = count - numChars;
    for (int i = ofs; i < count; i++) {
        float xpos = displayPos + (displayWidth / (float) count) * (float) i + ((displayWidth / 2.0) / (float) count);
        GLFont::getFont(fontSize).drawString(freqStr.str().substr(i - ofs, 1), xpos, 0, fontHeight, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
    }

    glColor4f(0.65, 0.65, 0.65, 0.25);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_LINES);
    for (int i = count; i >= 0; i--) {
        float xpos = displayPos + (displayWidth / (float) count) * (float) i;
        glVertex2f(xpos, -1.0);
        glVertex2f(xpos, 1.0);
    }
    glEnd();
    glDisable(GL_BLEND);
}


void TuningContext::DrawTunerDigitBox(int index, int count, float displayPos, float displayWidth, RGBA4f /* c */) {
    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];
    float pixelHeight = 2.0/viewHeight;

    glColor4f(1.0, 0,0,1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float xpos = displayPos + (displayWidth / (float) count) * (float) (count-index);
    float xpos2 = displayPos + (displayWidth / (float) count) * (float) ((count-1)-index);
    glBegin(GL_LINE_STRIP);
    glVertex2f(xpos, 1.0-pixelHeight);
    glVertex2f(xpos, -1.0+pixelHeight);
    glVertex2f(xpos2, -1.0+pixelHeight);
    glVertex2f(xpos2, 1.0-pixelHeight);
    glVertex2f(xpos, 1.0-pixelHeight);
    glEnd();
    glDisable(GL_BLEND);
}




int TuningContext::GetTunerDigitIndex(float mPos, int count, float displayPos, float displayWidth) {
    mPos -= 0.5;
    mPos *= 2.0;

    float delta = mPos - displayPos;

    if (delta < 0 || delta > displayWidth) {
        return 0;
    }

    int index = floor((delta / displayWidth) * (count));

    return count - index;
}

void TuningContext::DrawTunerBarIndexed(int start, int end, int count, float displayPos, float displayWidth, RGBA4f color, float /* alpha */, bool top,
bool bottom) {
    float ofs = (displayWidth / (float) count);
    float p2 = displayPos + ofs * (float) (count - start + 1);
    float p1 = displayPos + ofs * (float) (count - end);

    float r = color.r, g = color.g, b = color.b, a = 0.6;

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBegin(GL_QUADS);
    if (top) {
        glColor4f(r * 0.5, g * 0.5, b * 0.5, a);
        glVertex2f(p2, 1.0);
        glVertex2f(p1, 1.0);
        glColor4f(r, g, b, a);
        glVertex2f(p1, 0.0);
        glVertex2f(p2, 0.0);
    }
    if (bottom) {
        glColor4f(r, g, b, a);
        glVertex2f(p2, 0.0);
        glVertex2f(p1, 0.0);
        glColor4f(r * 0.5, g * 0.5, b * 0.5, a);
        glVertex2f(p1, -1.0);
        glVertex2f(p2, -1.0);
    }
    glEnd();
    glDisable(GL_BLEND);
}

void TuningContext::DrawDemodFreqBw(long long freq, unsigned int bw, long long center) {
    DrawTuner(freq, 11, -1.0, (1.0 / 3.0) * 2.0);
    DrawTuner(bw, 7, -1.0 + (2.25 / 3.0), (1.0 / 4.0) * 2.0);
    DrawTuner(center, 11, -1.0 + (2.0 / 3.0) * 2.0, (1.0 / 3.0) * 2.0);
}

