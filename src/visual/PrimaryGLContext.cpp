#include "PrimaryGLContext.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include "AppFrame.h"
#include <algorithm>

GLFont PrimaryGLContext::fonts[GLFONT_MAX];

wxString PrimaryGLContext::glGetwxString(GLenum name) {
    const GLubyte *v = glGetString(name);
    if (v == 0) {
        // The error is not important. It is GL_INVALID_ENUM.
        // We just want to clear the error stack.
        glGetError();

        return wxString();
    }

    return wxString((const char*) v);
}

void PrimaryGLContext::CheckGLError() {
    GLenum errLast = GL_NO_ERROR;

    for (;;) {
        GLenum err = glGetError();
        if (err == GL_NO_ERROR)
            return;

        if (err == errLast) {
            wxLogError
            (wxT("OpenGL error state couldn't be reset."));
            return;
        }

        errLast = err;

        wxLogError
        (wxT("OpenGL error %d"), err);
    }
}

PrimaryGLContext::PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext) :
        wxGLContext(canvas, sharedContext) {
    SetCurrent(*canvas);
    // Pre-load fonts
    for (int i = 0; i < GLFONT_MAX; i++) {
        getFont((GLFontSize) i);
    }
    CheckGLError();
}

GLFont &PrimaryGLContext::getFont(GLFontSize esize) {
    if (!fonts[esize].isLoaded()) {

        std::string fontName;
        switch (esize) {
        case GLFONT_SIZE12:
            fontName = "vera_sans_mono12.fnt";
            break;
        case GLFONT_SIZE16:
            fontName = "vera_sans_mono16.fnt";
            break;
        case GLFONT_SIZE18:
            fontName = "vera_sans_mono18.fnt";
            break;
        case GLFONT_SIZE24:
            fontName = "vera_sans_mono24.fnt";
            break;
        case GLFONT_SIZE32:
            fontName = "vera_sans_mono32.fnt";
            break;
        case GLFONT_SIZE48:
            fontName = "vera_sans_mono48.fnt";
            break;
        }

        fonts[esize].loadFont(fontName);
    }

    return fonts[esize];
}

void PrimaryGLContext::DrawDemodInfo(DemodulatorInstance *demod, float r, float g, float b, long long center_freq, long long srate) {
    if (!demod) {
        return;
    }
    if (!srate) {
        srate = wxGetApp().getSampleRate();
    }

    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];
    float viewWidth = (float) vp[2];

    if (center_freq == -1) {
        center_freq = wxGetApp().getFrequency();
    }

    float uxPos = (float) (demod->getFrequency() - (center_freq - srate / 2)) / (float) srate;
    uxPos = (uxPos - 0.5) * 2.0;

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
    glColor4f(r, g, b, 0.6);

    float ofs = ((float) demod->getBandwidth()) / (float) srate;


    float labelHeight = 20.0 / viewHeight;
    float hPos = -1.0 + labelHeight;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0, 0, 0, 0.35);
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofs, hPos + labelHeight, 0.0);
    glVertex3f(uxPos - ofs, -1.0, 0.0);

    glVertex3f(uxPos + ofs, -1.0, 0.0);
    glVertex3f(uxPos + ofs, hPos + labelHeight, 0.0);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);

    glColor4f(r, g, b, 0.2);
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofs, 1.0, 0.0);
    glVertex3f(uxPos - ofs, -1.0, 0.0);

    glVertex3f(uxPos + ofs, -1.0, 0.0);
    glVertex3f(uxPos + ofs, 1.0, 0.0);
    glEnd();



    if (ofs * 2.0 < 16.0 / viewWidth) {
        ofs = 16.0 / viewWidth;

        glColor4f(r, g, b, 0.2);
        glBegin(GL_QUADS);
        glVertex3f(uxPos - ofs, hPos + labelHeight, 0.0);
        glVertex3f(uxPos - ofs, -1.0, 0.0);

        glVertex3f(uxPos + ofs, -1.0, 0.0);
        glVertex3f(uxPos + ofs, hPos + labelHeight, 0.0);
        glEnd();
    }

    glColor4f(1.0, 1.0, 1.0, 0.8);

    getFont(PrimaryGLContext::GLFONT_SIZE16).drawString(demod->getLabel(), uxPos, hPos, 16, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);

    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawDemod(DemodulatorInstance *demod, float r, float g, float b, long long center_freq, long long srate) {
    if (!demod) {
        return;
    }
    if (!srate) {
        srate = wxGetApp().getSampleRate();
    }

    if (center_freq == -1) {
        center_freq = wxGetApp().getFrequency();
    }

    float uxPos = (float) (demod->getFrequency() - (center_freq - srate / 2)) / (float) srate;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
    glColor4f(r, g, b, 0.6);

    glBegin(GL_LINES);
    glVertex3f((uxPos - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0, -1.0, 0.0);

    float ofs = ((float) demod->getBandwidth()) / (float) srate;

    glVertex3f((uxPos - 0.5) * 2.0 - ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 - ofs, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 + ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 + ofs, -1.0, 0.0);

    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
    glColor4f(r, g, b, 0.2);
    glBegin(GL_QUADS);
    glVertex3f((uxPos - 0.5) * 2.0 - ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 - ofs, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 + ofs, -1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 + ofs, 1.0, 0.0);
    glEnd();

    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    float viewHeight = (float) vp[3];
    float viewWidth = (float) vp[2];

    float labelHeight = 20.0 / viewHeight;
    float xOfs = (2.0 / viewWidth);
    float yOfs = (2.0 / viewHeight);
    float hPos = labelHeight;

    glDisable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_COLOR_MATERIAL);

    glEnable(GL_BLEND);

    std::string demodStr;
    GLFont::Align demodAlign;

    switch (demod->getDemodulatorType()) {
    case DEMOD_TYPE_FM:
        demodStr = "FM";
        demodAlign = GLFont::GLFONT_ALIGN_CENTER;
        break;
    case DEMOD_TYPE_AM:
        demodStr = "AM";
        demodAlign = GLFont::GLFONT_ALIGN_CENTER;
        break;
    case DEMOD_TYPE_LSB:
        demodStr = "LSB";
        demodAlign = GLFont::GLFONT_ALIGN_RIGHT;
        break;
    case DEMOD_TYPE_USB:
        demodStr = "USB";
        demodAlign = GLFont::GLFONT_ALIGN_LEFT;
        break;
    }

    glColor3f(0, 0, 0);
    getFont(PrimaryGLContext::GLFONT_SIZE16).drawString(demodStr, 2.0 * (uxPos - 0.5) + xOfs, -1.0 + hPos - yOfs, 16, demodAlign,
            GLFont::GLFONT_ALIGN_CENTER);
    glColor3f(0.8, 0.8, 0.8);
    getFont(PrimaryGLContext::GLFONT_SIZE16).drawString(demodStr, 2.0 * (uxPos - 0.5), -1.0 + hPos, 16, demodAlign, GLFont::GLFONT_ALIGN_CENTER);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

}

void PrimaryGLContext::DrawFreqSelector(float uxPos, float r, float g, float b, float w, long long center_freq, long long srate) {
    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    long long bw = 0;

    if (!demod) {
        bw = defaultDemodParams.bandwidth;
    } else {
        bw = demod->getBandwidth();
    }

    if (!srate) {
        srate = wxGetApp().getSampleRate();
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_DST_COLOR);
    glColor4f(r, g, b, 0.6);

    glBegin(GL_LINES);
    glVertex3f((uxPos - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0, -1.0, 0.0);

    float ofs;

    if (w) {
        ofs = w;
    } else {
        ofs = ((float) bw) / (float) srate;
    }

    glVertex3f((uxPos - 0.5) * 2.0 - ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 - ofs, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 + ofs, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 + ofs, -1.0, 0.0);

    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

}

void PrimaryGLContext::BeginDraw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void PrimaryGLContext::EndDraw() {
    glFlush();

    CheckGLError();
}

