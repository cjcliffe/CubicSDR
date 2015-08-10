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
            std::cout << "OpenGL error state couldn't be reset." << std::endl;
            return;
        }

        errLast = err;

        std::cout << "OpenGL Error " << err << std::endl;
    }
}

PrimaryGLContext::PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext) :
        wxGLContext(canvas, sharedContext), hoverAlpha(1.0) {
//#ifndef __linux__
//    SetCurrent(*canvas);
//    // Pre-load fonts
//    for (int i = 0; i < GLFONT_MAX; i++) {
//        getFont((GLFontSize) i);
//    }
//    CheckGLError();
//#endif
}

void PrimaryGLContext::DrawDemodInfo(DemodulatorInstance *demod, RGB3f color, long long center_freq, long long srate) {
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(color.r, color.g, color.b, 0.6);

    float ofs = ((float) demod->getBandwidth()) / (float) srate;
    float ofsLeft = (demod->getDemodulatorType()!=DEMOD_TYPE_USB)?ofs:0, ofsRight = (demod->getDemodulatorType()!=DEMOD_TYPE_LSB)?ofs:0;

    float labelHeight = 20.0 / viewHeight;
    float hPos = -1.0 + labelHeight;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0, 0, 0, 0.35);
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofsLeft, hPos + labelHeight, 0.0);
    glVertex3f(uxPos - ofsLeft, -1.0, 0.0);

    glVertex3f(uxPos + ofsRight, -1.0, 0.0);
    glVertex3f(uxPos + ofsRight, hPos + labelHeight, 0.0);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glColor4f(color.r, color.g, color.b, 0.2);
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofsLeft, 1.0, 0.0);
    glVertex3f(uxPos - ofsLeft, -1.0, 0.0);

    glVertex3f(uxPos + ofsRight, -1.0, 0.0);
    glVertex3f(uxPos + ofsRight, 1.0, 0.0);
    glEnd();

    if (ofs * 2.0 < 16.0 / viewWidth) {
        glColor4f(color.r, color.g, color.b, 0.2);
        glBegin(GL_QUADS);
        glVertex3f(uxPos - ofsLeft, hPos + labelHeight, 0.0);
        glVertex3f(uxPos - ofsLeft, -1.0, 0.0);

        glVertex3f(uxPos + ofsRight, -1.0, 0.0);
        glVertex3f(uxPos + ofsRight, hPos + labelHeight, 0.0);
        glEnd();
    }


    glColor4f(1.0, 1.0, 1.0, 0.8);

    if (demod->getDemodulatorType() == DEMOD_TYPE_USB) {
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demod->getLabel(), uxPos, hPos, 16, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
    } else if (demod->getDemodulatorType() == DEMOD_TYPE_LSB) {
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demod->getLabel(), uxPos, hPos, 16, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
    } else {
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demod->getLabel(), uxPos, hPos, 16, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
    }

    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawDemod(DemodulatorInstance *demod, RGB3f color, long long center_freq, long long srate) {
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

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color.r, color.g, color.b, 0.6);

    float ofs = ((float) demod->getBandwidth()) / (float) srate;
    float ofsLeft = (demod->getDemodulatorType()!=DEMOD_TYPE_USB)?ofs:0, ofsRight = (demod->getDemodulatorType()!=DEMOD_TYPE_LSB)?ofs:0;

    glBegin(GL_LINES);
    glVertex3f((uxPos - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 - ofsLeft, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 - ofsLeft, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 + ofsRight, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 + ofsRight, -1.0, 0.0);

    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color.r, color.g, color.b, 0.2*hoverAlpha);
    glBegin(GL_QUADS);
    glVertex3f((uxPos - 0.5) * 2.0 - ofsLeft, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 - ofsLeft, -1.0, 0.0);

    glVertex3f((uxPos - 0.5) * 2.0 + ofsRight, -1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0 + ofsRight, 1.0, 0.0);
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

    std::string demodStr = "";
    GLFont::Align demodAlign = GLFont::GLFONT_ALIGN_CENTER;

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
        uxPos -= xOfs;
        break;
    case DEMOD_TYPE_USB:
        demodStr = "USB";
        demodAlign = GLFont::GLFONT_ALIGN_LEFT;
        uxPos += xOfs;
        break;
    }

    glColor3f(0, 0, 0);
    GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodStr, 2.0 * (uxPos - 0.5) + xOfs, -1.0 + hPos - yOfs, 16, demodAlign,
            GLFont::GLFONT_ALIGN_CENTER);
    glColor3f(0.8, 0.8, 0.8);
    GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodStr, 2.0 * (uxPos - 0.5), -1.0 + hPos, 16, demodAlign, GLFont::GLFONT_ALIGN_CENTER);

    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawFreqSelector(float uxPos, RGB3f color, float w, long long center_freq, long long srate) {
    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    long long bw = 0;

    int last_type = wxGetApp().getDemodMgr().getLastDemodulatorType();

    if (!demod) {
        bw = wxGetApp().getDemodMgr().getLastBandwidth();
    } else {
        bw = demod->getBandwidth();
    }

    if (!srate) {
        srate = wxGetApp().getSampleRate();
    }

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color.r, color.g, color.b, 0.6);

    glBegin(GL_LINES);

    glVertex3f((uxPos - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos - 0.5) * 2.0, -1.0, 0.0);

    float ofs;

    if (w) {
        ofs = w;
    } else {
        ofs = ((float) bw) / (float) srate;
    }

    if (last_type != DEMOD_TYPE_USB) {
        glVertex3f((uxPos - 0.5) * 2.0 - ofs, 1.0, 0.0);
        glVertex3f((uxPos - 0.5) * 2.0 - ofs, -1.0, 0.0);
    }

    if (last_type != DEMOD_TYPE_LSB) {
        glVertex3f((uxPos - 0.5) * 2.0 + ofs, 1.0, 0.0);
        glVertex3f((uxPos - 0.5) * 2.0 + ofs, -1.0, 0.0);
    }

    glEnd();
    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawRangeSelector(float uxPos1, float uxPos2, RGB3f color) {
    if (uxPos2 < uxPos1) {
        float temp = uxPos2;
        uxPos2=uxPos1;
        uxPos1=temp;
    }

    int last_type = wxGetApp().getDemodMgr().getLastDemodulatorType();

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color.r, color.g, color.b, 0.6);

    glLineWidth((last_type == DEMOD_TYPE_USB)?2.0:1.0);

    glBegin(GL_LINES);
    glVertex3f((uxPos1 - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos1 - 0.5) * 2.0, -1.0, 0.0);
    glEnd();

    glLineWidth((last_type == DEMOD_TYPE_LSB)?2.0:1.0);

    glBegin(GL_LINES);
    glVertex3f((uxPos2 - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos2 - 0.5) * 2.0, -1.0, 0.0);
    glEnd();

    glLineWidth(1.0);

    glDisable(GL_BLEND);
}

void PrimaryGLContext::BeginDraw(float r, float g, float b) {
    glClearColor(r,g,b, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void PrimaryGLContext::EndDraw() {
    glFlush();

    CheckGLError();
}

void PrimaryGLContext::setHoverAlpha(float hoverAlpha) {
    this->hoverAlpha = hoverAlpha;
}
