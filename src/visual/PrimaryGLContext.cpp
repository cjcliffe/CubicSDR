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

void PrimaryGLContext::DrawDemodInfo(DemodulatorInstance *demod, RGBA4f color, long long center_freq, long long srate) {
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
    
    long long demodFreq = demod->getFrequency();

    if (demod->isDeltaLock()) {
        demodFreq = wxGetApp().getFrequency() + demod->getDeltaLockOfs();
    }
    
    float uxPos = (float) (demodFreq - (center_freq - srate / 2)) / (float) srate;
    uxPos = (uxPos - 0.5) * 2.0;

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(color.r, color.g, color.b, 0.6);

    float ofs = ((float) demod->getBandwidth()) / (float) srate;
    float ofsLeft = (demod->getDemodulatorType()!="USB")?ofs:0, ofsRight = (demod->getDemodulatorType()!="LSB")?ofs:0;

    float labelHeight = 20.0 / viewHeight;
    float hPos = -1.0 + labelHeight;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool soloMode = wxGetApp().getSoloMode();
    bool isSolo = soloMode && demod == wxGetApp().getDemodMgr().getLastActiveDemodulator();
    
    if (isSolo) {
        glColor4f(0.8, 0.8, 0, 0.35);
    } else if (demod->isMuted()) {
        glColor4f(0.8, 0, 0, 0.35);
    } else if (soloMode) {
        glColor4f(0.2, 0, 0, 0.35);
    } else {
        glColor4f(0, 0, 0, 0.35);
    }
    
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

    std::string demodLabel = demod->getLabel();
    
    if (demod->isMuted()) {
        demodLabel = std::string("[M] ") + demodLabel;
    } else if (isSolo) {
        demodLabel = std::string("[S] ") + demodLabel;
    }
    
    if (demod->isDeltaLock()) {
        demodLabel.append(" [V]");
    }
    
    if (demod->getDemodulatorType() == "USB") {
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos, hPos, 16, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
    } else if (demod->getDemodulatorType() == "LSB") {
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos, hPos, 16, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
    } else {
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos, hPos, 16, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
    }

    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawFreqBwInfo(long long freq, int bw, RGBA4f color, long long center_freq, long long srate, bool stack) {
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
    
    float uxPos = (float) (freq - (center_freq - srate / 2)) / (float) srate;
    uxPos = (uxPos - 0.5) * 2.0;

    std::string lastType = wxGetApp().getDemodMgr().getLastDemodulatorType();

    float ofs = (float) bw / (float) srate;
    float ofsLeft = (lastType!="USB")?ofs:0, ofsRight = (lastType!="LSB")?ofs:0;

    float labelHeight = 20.0 / viewHeight;
    float hPos = -1.0 + (stack?(labelHeight*3.0):labelHeight);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0, 0, 0, 0.35);
    
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofsLeft, hPos + labelHeight, 0.0);
    glVertex3f(uxPos - ofsLeft, -1.0, 0.0);
    
    glVertex3f(uxPos + ofsRight, -1.0, 0.0);
    glVertex3f(uxPos + ofsRight, hPos + labelHeight, 0.0);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    glColor4f(color.r, color.g, color.b, 0.1);
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofsLeft, 1.0, 0.0);
    glVertex3f(uxPos - ofsLeft, -1.0, 0.0);
    
    glVertex3f(uxPos + ofsRight, -1.0, 0.0);
    glVertex3f(uxPos + ofsRight, 1.0, 0.0);
    glEnd();
    
    if (ofs * 2.0 < 16.0 / viewWidth) {
        glColor4f(color.r, color.g, color.b, 0.1);
        glBegin(GL_QUADS);
        glVertex3f(uxPos - ofsLeft, hPos + labelHeight, 0.0);
        glVertex3f(uxPos - ofsLeft, -1.0, 0.0);
        
        glVertex3f(uxPos + ofsRight, -1.0, 0.0);
        glVertex3f(uxPos + ofsRight, hPos + labelHeight, 0.0);
        glEnd();
    }
    
    glColor4f(1.0, 1.0, 1.0, 0.8);
    
    std::string demodLabel = std::to_string((double)freq/1000000.0);
    
    double shadowOfsX = 4.0 / viewWidth, shadowOfsY = 2.0 / viewHeight;
    
    if (lastType == "USB") {
        glColor4f(0,0,0, 1.0);
        glBlendFunc(GL_ONE, GL_ZERO);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos+shadowOfsX, hPos+shadowOfsY, 16, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos-shadowOfsX, hPos-shadowOfsY, 16, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos, hPos, 16, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
    } else if (lastType == "LSB") {
        glBlendFunc(GL_ONE, GL_ZERO);
        glColor4f(0,0,0, 1.0);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos+shadowOfsX, hPos+shadowOfsY, 16, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos-shadowOfsX, hPos-shadowOfsY, 16, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos, hPos, 16, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
    } else {
        glBlendFunc(GL_ONE, GL_ZERO);
        glColor4f(0,0,0, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos+shadowOfsX, hPos+shadowOfsY, 16, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos-shadowOfsX, hPos-shadowOfsY, 16, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodLabel, uxPos, hPos, 16, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
    }
    
    glDisable(GL_BLEND);
}

void PrimaryGLContext::DrawDemod(DemodulatorInstance *demod, RGBA4f color, long long center_freq, long long srate) {
    if (!demod) {
        return;
    }
    if (!srate) {
        srate = wxGetApp().getSampleRate();
    }

    if (center_freq == -1) {
        center_freq = wxGetApp().getFrequency();
    }

    long long demodFreq = demod->getFrequency();

    if (demod->isDeltaLock()) {
        demodFreq = wxGetApp().getFrequency() + demod->getDeltaLockOfs();
    }
    
    float uxPos = (float) (demodFreq - (center_freq - srate / 2)) / (float) srate;

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color.r, color.g, color.b, 0.6);

    float ofs = ((float) demod->getBandwidth()) / (float) srate;
    float ofsLeft = (demod->getDemodulatorType()!="USB")?ofs:0, ofsRight = (demod->getDemodulatorType()!="LSB")?ofs:0;

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

    demodStr = demod->getDemodulatorType();

    demodAlign = GLFont::GLFONT_ALIGN_CENTER;

    if (demodStr == "LSB") {
        demodAlign = GLFont::GLFONT_ALIGN_RIGHT;
        uxPos -= xOfs;
    } else if (demodStr == "USB") {
        demodAlign = GLFont::GLFONT_ALIGN_LEFT;
        uxPos += xOfs;
    }
    // advanced demodulators start here

//	if (demod->getDemodulatorCons() > 0) {
//		demodStr = demodStr + std::to_string(demod->getDemodulatorCons());
//	}
    
    // add lock to string if we have an lock
    if(demod->getDemodulatorLock()) {
        demodStr = demodStr + " Lock";
    } 
    // else {
    //     demodStr = demodStr + " UnLock";
    // }

    glColor3f(0, 0, 0);
    GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodStr, 2.0 * (uxPos - 0.5) + xOfs, -1.0 + hPos - yOfs, 16, demodAlign,
            GLFont::GLFONT_ALIGN_CENTER);
    glColor3f(1, 1, 1);
    GLFont::getFont(GLFont::GLFONT_SIZE16).drawString(demodStr, 2.0 * (uxPos - 0.5), -1.0 + hPos, 16, demodAlign, GLFont::GLFONT_ALIGN_CENTER);

    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawFreqSelector(float uxPos, RGBA4f color, float w, long long /* center_freq */, long long srate) {
    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    long long bw = 0;

    std::string last_type = wxGetApp().getDemodMgr().getLastDemodulatorType();

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

    if (last_type != "USB") {
        glVertex3f((uxPos - 0.5) * 2.0 - ofs, 1.0, 0.0);
        glVertex3f((uxPos - 0.5) * 2.0 - ofs, -1.0, 0.0);
    }

    if (last_type != "LSB") {
        glVertex3f((uxPos - 0.5) * 2.0 + ofs, 1.0, 0.0);
        glVertex3f((uxPos - 0.5) * 2.0 + ofs, -1.0, 0.0);
    }

    glEnd();
    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawRangeSelector(float uxPos1, float uxPos2, RGBA4f color) {
    if (uxPos2 < uxPos1) {
        float temp = uxPos2;
        uxPos2=uxPos1;
        uxPos1=temp;
    }

    std::string last_type = wxGetApp().getDemodMgr().getLastDemodulatorType();

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color.r, color.g, color.b, 0.6);

    glLineWidth((last_type == "USB")?2.0:1.0);

    glBegin(GL_LINES);
    glVertex3f((uxPos1 - 0.5) * 2.0, 1.0, 0.0);
    glVertex3f((uxPos1 - 0.5) * 2.0, -1.0, 0.0);
    glEnd();

    glLineWidth((last_type == "LSB")?2.0:1.0);

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
