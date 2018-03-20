// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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

PrimaryGLContext::PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs* ctxAttrs) :
        wxGLContext(canvas, sharedContext, (const wxGLContextAttrs*) ctxAttrs), hoverAlpha(1.0) {



//#ifndef __linux__
//    SetCurrent(*canvas);
//    // Pre-load fonts
//    for (int i = 0; i < GLFONT_MAX; i++) {
//        getFont((GLFontSize) i);
//    }
//    CheckGLError();
//#endif
}

void PrimaryGLContext::DrawDemodInfo(DemodulatorInstancePtr demod, RGBA4f color, long long center_freq, long long srate, bool centerline) {
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
    glColor4f(color.r, color.g, color.b, 0.6f);

    float ofs = ((float) demod->getBandwidth()) / (float) srate;
    float ofsLeft = (demod->getDemodulatorType()!="USB")?ofs:0, ofsRight = (demod->getDemodulatorType()!="LSB")?ofs:0;

    float labelHeight = 20.0 / viewHeight;
    float hPos = -1.0 + labelHeight;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool soloMode = wxGetApp().getSoloMode();
    bool isRecording = demod->isRecording();
    bool isSolo = soloMode && demod == wxGetApp().getDemodMgr().getLastActiveDemodulator();
    
    RGBA4f labelBg(0, 0, 0, 0.35f);

    if (isSolo) {
        labelBg.r = labelBg.g = 0.8f;
    } else if (demod->isMuted()) {
        labelBg.r = 0.8f;
    } else if (soloMode) {
        labelBg.r = 0.2f;
    }

    // TODO: Better recording indicator... pulsating red circle?
    if (isRecording) {
        auto t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        labelBg.g = sinf(2.0f * M_PI * (float(t) / 1000.0f)) * 0.25f + 0.75f;
    }

    glColor4f(labelBg.r, labelBg.g, labelBg.b, labelBg.a);
    
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofsLeft, hPos + labelHeight, 0.0);
    glVertex3f(uxPos - ofsLeft, -1.0, 0.0);

    glVertex3f(uxPos + ofsRight, -1.0, 0.0);
    glVertex3f(uxPos + ofsRight, hPos + labelHeight, 0.0);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glColor4f(color.r, color.g, color.b, 0.2f);
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofsLeft, 1.0, 0.0);
    glVertex3f(uxPos - ofsLeft, -1.0, 0.0);

    glVertex3f(uxPos + ofsRight, -1.0, 0.0);
    glVertex3f(uxPos + ofsRight, 1.0, 0.0);
    glEnd();

    if (ofs * 2.0 < 16.0 / viewWidth) {
        glColor4f(color.r, color.g, color.b, 0.2f);
        glBegin(GL_QUADS);
        glVertex3f(uxPos - ofsLeft, hPos + labelHeight, 0.0);
        glVertex3f(uxPos - ofsLeft, -1.0, 0.0);

        glVertex3f(uxPos + ofsRight, -1.0, 0.0);
        glVertex3f(uxPos + ofsRight, hPos + labelHeight, 0.0);
        glEnd();
    }

    if (centerline) {
        glColor4f(color.r, color.g, color.b, 0.5);
        glBegin(GL_LINES);
        glVertex3f(uxPos, 1.0, 0.0);
        glVertex3f(uxPos, -1.0, 0.0);
        glEnd();
    }

    glColor4f(1.0, 1.0, 1.0, 0.8f);

    std::string demodLabel, demodPrefix;

    if (demod->isDeltaLock()) {
        demodPrefix.append("V");
    }

    if (isRecording) {
        demodPrefix.append("R");
    }

    if (demod->isMuted()) {
        demodPrefix.append("M");
    } else if (isSolo) {
        demodPrefix.append("S");
    }

    // Set the prefix
    if (!demodPrefix.empty()) {
        demodLabel = "[" + demodPrefix + "] ";
    }    
    // Append the default label
    demodLabel.append(demod->getLabel());

    if (demod->getDemodulatorType() == "USB") {
        GLFont::getFont(16, GLFont::getScaleFactor()).drawString(demodLabel, uxPos, hPos, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    } else if (demod->getDemodulatorType() == "LSB") {
        GLFont::getFont(16, GLFont::getScaleFactor()).drawString(demodLabel, uxPos, hPos, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    } else {
        GLFont::getFont(16, GLFont::getScaleFactor()).drawString(demodLabel, uxPos, hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    }

    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawFreqBwInfo(long long freq, int bw, RGBA4f color, long long center_freq, long long srate, bool stack, bool centerline) {
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

    glColor4f(0, 0, 0, 0.35f);
    
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofsLeft, hPos + labelHeight, 0.0);
    glVertex3f(uxPos - ofsLeft, -1.0, 0.0);
    
    glVertex3f(uxPos + ofsRight, -1.0, 0.0);
    glVertex3f(uxPos + ofsRight, hPos + labelHeight, 0.0);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    glColor4f(color.r, color.g, color.b, 0.1f);
    glBegin(GL_QUADS);
    glVertex3f(uxPos - ofsLeft, 1.0, 0.0);
    glVertex3f(uxPos - ofsLeft, -1.0, 0.0);
    
    glVertex3f(uxPos + ofsRight, -1.0, 0.0);
    glVertex3f(uxPos + ofsRight, 1.0, 0.0);
    glEnd();
    
    if (ofs * 2.0 < 16.0 / viewWidth) {
        glColor4f(color.r, color.g, color.b, 0.1f);
        glBegin(GL_QUADS);
        glVertex3f(uxPos - ofsLeft, hPos + labelHeight, 0.0);
        glVertex3f(uxPos - ofsLeft, -1.0, 0.0);
        
        glVertex3f(uxPos + ofsRight, -1.0, 0.0);
        glVertex3f(uxPos + ofsRight, hPos + labelHeight, 0.0);
        glEnd();
    }

    if (centerline) {
        glColor4f(color.r, color.g, color.b, 0.5);
        glBegin(GL_LINES);
        glVertex3f(uxPos, 1.0, 0.0);
        glVertex3f(uxPos, -1.0, 0.0);
        glEnd();
    }
    
    glColor4f(1.0, 1.0, 1.0, 0.8f);
    
    std::string demodLabel = std::to_string((double)freq/1000000.0);
    
    double shadowOfsX = 4.0 / viewWidth, shadowOfsY = 2.0 / viewHeight;

    GLFont::Drawer refDrawingFont = GLFont::getFont(16, GLFont::getScaleFactor());
    
    if (lastType == "USB") {
        glColor4f(0,0,0, 1.0);
        glBlendFunc(GL_ONE, GL_ZERO);
        refDrawingFont.drawString(demodLabel, uxPos+shadowOfsX, hPos+shadowOfsY, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
        refDrawingFont.drawString(demodLabel, uxPos-shadowOfsX, hPos-shadowOfsY, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        refDrawingFont.drawString(demodLabel, uxPos, hPos, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
    } else if (lastType == "LSB") {
        glBlendFunc(GL_ONE, GL_ZERO);
        glColor4f(0,0,0, 1.0);
        refDrawingFont.drawString(demodLabel, uxPos+shadowOfsX, hPos+shadowOfsY, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
        refDrawingFont.drawString(demodLabel, uxPos-shadowOfsX, hPos-shadowOfsY, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        refDrawingFont.drawString(demodLabel, uxPos, hPos, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
    } else {
        glBlendFunc(GL_ONE, GL_ZERO);
        glColor4f(0,0,0, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        refDrawingFont.drawString(demodLabel, uxPos+shadowOfsX, hPos+shadowOfsY, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
        refDrawingFont.drawString(demodLabel, uxPos-shadowOfsX, hPos-shadowOfsY, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        refDrawingFont.drawString(demodLabel, uxPos, hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
    }
    
    glDisable(GL_BLEND);
}

void PrimaryGLContext::DrawDemod(DemodulatorInstancePtr demod, RGBA4f color, long long center_freq, long long srate) {
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
    glColor4f(color.r, color.g, color.b, 0.6f);

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

    //Displayed string is wstring, so use wxString to do the heavy lifting of converting  getDemodulatorType()...
    wxString demodStr;

    demodStr.assign(demod->getDemodulatorType());

    if (demodStr == "LSB") {
        uxPos -= xOfs;
    } else if (demodStr == "USB") {
        uxPos += xOfs;
    }
    // advanced demodulators start here

//	if (demod->getDemodulatorCons() > 0) {
//		demodStr = demodStr + std::to_string(demod->getDemodulatorCons());
//	}
    
    // add lock to string if we have an lock
    if(demod->getDemodulatorLock()) {
        demodStr += " Lock";
    }

    // else {
    //     demodStr = demodStr + " UnLock";
    // }

    //Shift the user label from the modem label more for the bigger 
    //font sizes so they do not step on each other...
    double heightShift = GLFont::getScaleFactor();

    //demodulator user label if present: type is displayed above the label, which is at the bottom of the screen.
    if (!demod->getDemodulatorUserLabel().empty()) {
        drawSingleDemodLabel(demodStr.ToStdWstring(), uxPos, hPos * 1.2 + hPos * 1.2 * heightShift, xOfs, yOfs, GLFont::GLFONT_ALIGN_CENTER);
        drawSingleDemodLabel(demod->getDemodulatorUserLabel(), uxPos, hPos * 1.2, xOfs, yOfs, GLFont::GLFONT_ALIGN_CENTER);
    }
    else {
        drawSingleDemodLabel(demodStr.ToStdWstring(), uxPos, hPos * 1.2, xOfs, yOfs, GLFont::GLFONT_ALIGN_CENTER);
    }

    glDisable(GL_BLEND);
}

void PrimaryGLContext::drawSingleDemodLabel(const std::wstring& demodStr, float uxPos, float hPos, float xOfs, float yOfs, GLFont::Align demodAlign) {

    GLFont::Drawer refDrawingFont = GLFont::getFont(16, GLFont::getScaleFactor());

    glColor3f(0, 0, 0);
    refDrawingFont.drawString(demodStr, 2.0 * (uxPos - 0.5) + xOfs, -1.0 + hPos - yOfs, demodAlign, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    
    glColor3f(1, 1, 1);
    refDrawingFont.drawString(demodStr, 2.0 * (uxPos - 0.5), -1.0 + hPos, demodAlign, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
}

void PrimaryGLContext::DrawFreqSelector(float uxPos, RGBA4f color, float w, long long /* center_freq */, long long srate) {
    
    DemodulatorInstancePtr demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

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
    glColor4f(color.r, color.g, color.b, 0.6f);

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
    glColor4f(color.r, color.g, color.b, 0.6f);

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
//    glFlush();

//    CheckGLError();
}

void PrimaryGLContext::setHoverAlpha(float hoverAlpha) {
    this->hoverAlpha = hoverAlpha;
}
