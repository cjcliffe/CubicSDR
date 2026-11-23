// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "PrimaryGLContext.h"
#include "DemodMarkers.h"
#include <chrono>

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"

wxString PrimaryGLContext::glGetwxString(GLenum name) {
    const GLubyte *v = glGetString(name);
    if (v == nullptr) {
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

bool PrimaryGLContext::SetCurrent(const wxGLCanvas& win) const {
    bool result = wxGLContext::SetCurrent(win);
#ifdef _WIN32
    // Swap interval is per-window state, and each context activates only its
    // own canvas, so one successful application lasts the context's life.
    if (result && wglSwapIntervalEXT && !swapIntervalApplied) {
        wglSwapIntervalEXT(0);
        swapIntervalApplied = true;
    }
#endif
    return result;
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

void PrimaryGLContext::DrawDemodInfo(const DemodulatorInstancePtr& demod, const RGBA4f& color, long long center_freq, long long srate, bool centerline) {
    if (!demod) {
        return;
    }
    if (!srate) {
        srate = wxGetApp().getSampleRate();
    }

    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    if (center_freq == -1) {
        center_freq = wxGetApp().getFrequency();
    }

    long long demodFreq = demod->getFrequency();

    if (demod->isDeltaLock()) {
        demodFreq = wxGetApp().getFrequency() + demod->getDeltaLockOfs();
    }

    bool soloMode = wxGetApp().getSoloMode();
    bool isRecording = demod->isRecording();
    bool isSolo = soloMode && demod == wxGetApp().getDemodMgr().getCurrentModem();

    long long timeMs = 0;
    // TODO: Better recording indicator... pulsating red circle?
    if (isRecording) {
        timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    DemodMarkers::MarkerInput in;
    in.freqHz = demodFreq;
    in.bandwidthHz = demod->getBandwidth();
    in.sideband = DemodMarkers::sidebandForType(demod->getDemodulatorType());
    in.viewCenterHz = center_freq;
    in.viewSpanHz = srate;
    in.viewWidthPx = vp[2];
    in.viewHeightPx = vp[3];
    in.deltaLocked = demod->isDeltaLock();
    in.muted = demod->isMuted();
    in.solo = isSolo;
    in.soloMode = soloMode;
    in.recording = isRecording;
    in.timeMs = timeMs;
    DemodMarkers::MarkerGeometry geo = DemodMarkers::markerGeometry(in);

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(color.r, color.g, color.b, 0.6f);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(geo.bg.r, geo.bg.g, geo.bg.b, geo.bg.a);
    float labelVerts[12] = {
        geo.label.x0, geo.label.y1, 0.0, geo.label.x0, geo.label.y0, 0.0,
        geo.label.x1, geo.label.y0, 0.0, geo.label.x1, geo.label.y1, 0.0
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, labelVerts);
    glDrawArrays(GL_QUADS, 0, 4);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glColor4f(color.r, color.g, color.b, 0.2f);
    float bandVerts[12] = {
        geo.band.x0, geo.band.y1, 0.0, geo.band.x0, geo.band.y0, 0.0,
        geo.band.x1, geo.band.y0, 0.0, geo.band.x1, geo.band.y1, 0.0
    };
    glVertexPointer(3, GL_FLOAT, 0, bandVerts);
    glDrawArrays(GL_QUADS, 0, 4);

    if (geo.narrowBand) {
        glColor4f(color.r, color.g, color.b, 0.2f);
        float narrowVerts[12] = {
            geo.label.x0, geo.label.y1, 0.0, geo.label.x0, geo.label.y0, 0.0,
            geo.label.x1, geo.label.y0, 0.0, geo.label.x1, geo.label.y1, 0.0
        };
        glVertexPointer(3, GL_FLOAT, 0, narrowVerts);
        glDrawArrays(GL_QUADS, 0, 4);
    }

    if (centerline) {
        glColor4f(color.r, color.g, color.b, 0.5);
        float centerVerts[6] = { geo.uxPos, 1.0, 0.0, geo.uxPos, -1.0, 0.0 };
        glVertexPointer(3, GL_FLOAT, 0, centerVerts);
        glDrawArrays(GL_LINES, 0, 2);
    }
    glDisableClientState(GL_VERTEX_ARRAY);

    glColor4f(1.0, 1.0, 1.0, 0.8f);

    std::string demodLabel = geo.prefix;
    demodLabel.append(demod->getLabel());

    if (geo.align == DemodMarkers::LabelAlign::Left) {
        GLFont::getFont(16, GLFont::getScaleFactor()).drawString(demodLabel, geo.uxPos, geo.hPos, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    } else if (geo.align == DemodMarkers::LabelAlign::Right) {
        GLFont::getFont(16, GLFont::getScaleFactor()).drawString(demodLabel, geo.uxPos, geo.hPos, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    } else {
        GLFont::getFont(16, GLFont::getScaleFactor()).drawString(demodLabel, geo.uxPos, geo.hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, 0, 0, true);
    }

    glDisable(GL_BLEND);
}

void PrimaryGLContext::DrawFreqBwInfo(long long freq, int bw, const RGBA4f& color, long long center_freq, long long srate, bool stack, bool centerline) {
    if (!srate) {
        srate = wxGetApp().getSampleRate();
    }

    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    if (center_freq == -1) {
        center_freq = wxGetApp().getFrequency();
    }

    std::string lastType = wxGetApp().getDemodMgr().getLastDemodulatorType();

    DemodMarkers::MarkerInput in;
    in.freqHz = freq;
    in.bandwidthHz = bw;
    in.sideband = DemodMarkers::sidebandForType(lastType);
    in.viewCenterHz = center_freq;
    in.viewSpanHz = srate;
    in.viewWidthPx = vp[2];
    in.viewHeightPx = vp[3];
    in.stackLabels = stack;
    DemodMarkers::MarkerGeometry geo = DemodMarkers::markerGeometry(in);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0, 0, 0, 0.35f);
    float labelVerts[12] = {
        geo.label.x0, geo.label.y1, 0.0, geo.label.x0, geo.label.y0, 0.0,
        geo.label.x1, geo.label.y0, 0.0, geo.label.x1, geo.label.y1, 0.0
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, labelVerts);
    glDrawArrays(GL_QUADS, 0, 4);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glColor4f(color.r, color.g, color.b, 0.1f);
    float bandVerts[12] = {
        geo.band.x0, geo.band.y1, 0.0, geo.band.x0, geo.band.y0, 0.0,
        geo.band.x1, geo.band.y0, 0.0, geo.band.x1, geo.band.y1, 0.0
    };
    glVertexPointer(3, GL_FLOAT, 0, bandVerts);
    glDrawArrays(GL_QUADS, 0, 4);

    if (geo.narrowBand) {
        glColor4f(color.r, color.g, color.b, 0.1f);
        float narrowVerts[12] = {
            geo.label.x0, geo.label.y1, 0.0, geo.label.x0, geo.label.y0, 0.0,
            geo.label.x1, geo.label.y0, 0.0, geo.label.x1, geo.label.y1, 0.0
        };
        glVertexPointer(3, GL_FLOAT, 0, narrowVerts);
        glDrawArrays(GL_QUADS, 0, 4);
    }

    if (centerline) {
        glColor4f(color.r, color.g, color.b, 0.5);
        float centerVerts[6] = { geo.uxPos, 1.0, 0.0, geo.uxPos, -1.0, 0.0 };
        glVertexPointer(3, GL_FLOAT, 0, centerVerts);
        glDrawArrays(GL_LINES, 0, 2);
    }
    glDisableClientState(GL_VERTEX_ARRAY);

    glColor4f(1.0, 1.0, 1.0, 0.8f);

    std::string demodLabel = geo.freqText;

    GLFont::Drawer refDrawingFont = GLFont::getFont(16, GLFont::getScaleFactor());

    if (geo.align == DemodMarkers::LabelAlign::Left) {
        glColor4f(0,0,0, 1.0);
        glBlendFunc(GL_ONE, GL_ZERO);
        refDrawingFont.drawString(demodLabel, geo.uxPos+geo.shadowDx, geo.hPos+geo.shadowDy, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
        refDrawingFont.drawString(demodLabel, geo.uxPos-geo.shadowDx, geo.hPos-geo.shadowDy, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        refDrawingFont.drawString(demodLabel, geo.uxPos, geo.hPos, GLFont::GLFONT_ALIGN_LEFT, GLFont::GLFONT_ALIGN_CENTER);
    } else if (geo.align == DemodMarkers::LabelAlign::Right) {
        glBlendFunc(GL_ONE, GL_ZERO);
        glColor4f(0,0,0, 1.0);
        refDrawingFont.drawString(demodLabel, geo.uxPos+geo.shadowDx, geo.hPos+geo.shadowDy, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
        refDrawingFont.drawString(demodLabel, geo.uxPos-geo.shadowDx, geo.hPos-geo.shadowDy, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        refDrawingFont.drawString(demodLabel, geo.uxPos, geo.hPos, GLFont::GLFONT_ALIGN_RIGHT, GLFont::GLFONT_ALIGN_CENTER);
    } else {
        glBlendFunc(GL_ONE, GL_ZERO);
        glColor4f(0,0,0, 1.0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        refDrawingFont.drawString(demodLabel, geo.uxPos+geo.shadowDx, geo.hPos+geo.shadowDy, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
        refDrawingFont.drawString(demodLabel, geo.uxPos-geo.shadowDx, geo.hPos-geo.shadowDy, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
        glColor4f(color.r, color.g, color.b, 1.0);
        refDrawingFont.drawString(demodLabel, geo.uxPos, geo.hPos, GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER);
    }

    glDisable(GL_BLEND);
}

void PrimaryGLContext::DrawDemod(const DemodulatorInstancePtr& demod, const RGBA4f& color, long long center_freq, long long srate) {
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

    float uxUnit = (float) (demodFreq - (center_freq - srate / 2)) / (float) srate;

    GLint vp[4];
    glGetIntegerv( GL_VIEWPORT, vp);

    DemodMarkers::MarkerInput in;
    in.freqHz = demodFreq;
    in.bandwidthHz = demod->getBandwidth();
    in.sideband = DemodMarkers::sidebandForType(demod->getDemodulatorType());
    in.viewCenterHz = center_freq;
    in.viewSpanHz = srate;
    in.viewWidthPx = vp[2];
    in.viewHeightPx = vp[3];
    DemodMarkers::MarkerGeometry geo = DemodMarkers::markerGeometry(in);

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color.r, color.g, color.b, 0.6f);

    float edgeVerts[18] = {
        geo.uxPos, 1.0, 0.0, geo.uxPos, -1.0, 0.0,
        geo.uxPos - geo.ofsLeft, 1.0, 0.0, geo.uxPos - geo.ofsLeft, -1.0, 0.0,
        geo.uxPos + geo.ofsRight, 1.0, 0.0, geo.uxPos + geo.ofsRight, -1.0, 0.0
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, edgeVerts);
    glDrawArrays(GL_LINES, 0, 6);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(color.r, color.g, color.b, 0.2*hoverAlpha);
    float hoverVerts[12] = {
        geo.uxPos - geo.ofsLeft, 1.0, 0.0, geo.uxPos - geo.ofsLeft, -1.0, 0.0,
        geo.uxPos + geo.ofsRight, -1.0, 0.0, geo.uxPos + geo.ofsRight, 1.0, 0.0
    };
    glVertexPointer(3, GL_FLOAT, 0, hoverVerts);
    glDrawArrays(GL_QUADS, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);

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

    demodStr.assign(DemodMarkers::demodTypeLabel(demod->getDemodulatorType(), demod->getDemodulatorLock() != 0));

    if (demodStr == "LSB") {
        uxUnit -= xOfs;
    } else if (demodStr == "USB") {
        uxUnit += xOfs;
    }
    // advanced demodulators start here

//	if (demod->getDemodulatorCons() > 0) {
//		demodStr = demodStr + std::to_string(demod->getDemodulatorCons());
//	}

    // else {
    //     demodStr = demodStr + " UnLock";
    // }

    //Shift the user label from the modem label more for the bigger 
    //font sizes so they do not step on each other...
    double heightShift = GLFont::getScaleFactor();

    //demodulator user label if present: type is displayed above the label, which is at the bottom of the screen.
    if (!demod->getDemodulatorUserLabel().empty()) {
        drawSingleDemodLabel(demodStr.ToStdWstring(), uxUnit, hPos * 1.2 + hPos * 1.2 * heightShift, xOfs, yOfs, GLFont::GLFONT_ALIGN_CENTER);
        drawSingleDemodLabel(demod->getDemodulatorUserLabel(), uxUnit, hPos * 1.2, xOfs, yOfs, GLFont::GLFONT_ALIGN_CENTER);
    }
    else {
        drawSingleDemodLabel(demodStr.ToStdWstring(), uxUnit, hPos * 1.2, xOfs, yOfs, GLFont::GLFONT_ALIGN_CENTER);
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

void PrimaryGLContext::DrawFreqSelector(float uxPos, const RGBA4f& color, float w, long long /* center_freq */, long long srate) {
    
    DemodulatorInstancePtr demod = wxGetApp().getDemodMgr().getCurrentModem();

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

    float ofs;

    if (w) {
        ofs = w;
    } else {
        ofs = ((float) bw) / (float) srate;
    }

    float selVerts[18];
    int selCount = 0;
    selVerts[selCount++] = (uxPos - 0.5) * 2.0; selVerts[selCount++] = 1.0; selVerts[selCount++] = 0.0;
    selVerts[selCount++] = (uxPos - 0.5) * 2.0; selVerts[selCount++] = -1.0; selVerts[selCount++] = 0.0;

    if (last_type != "USB") {
        selVerts[selCount++] = (uxPos - 0.5) * 2.0 - ofs; selVerts[selCount++] = 1.0; selVerts[selCount++] = 0.0;
        selVerts[selCount++] = (uxPos - 0.5) * 2.0 - ofs; selVerts[selCount++] = -1.0; selVerts[selCount++] = 0.0;
    }

    if (last_type != "LSB") {
        selVerts[selCount++] = (uxPos - 0.5) * 2.0 + ofs; selVerts[selCount++] = 1.0; selVerts[selCount++] = 0.0;
        selVerts[selCount++] = (uxPos - 0.5) * 2.0 + ofs; selVerts[selCount++] = -1.0; selVerts[selCount++] = 0.0;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, selVerts);
    glDrawArrays(GL_LINES, 0, selCount / 3);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);

}

void PrimaryGLContext::DrawRangeSelector(float uxPos1, float uxPos2, const RGBA4f& color) {
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

    float rangeVerts[6];
    glEnableClientState(GL_VERTEX_ARRAY);
    rangeVerts[0] = (uxPos1 - 0.5) * 2.0; rangeVerts[1] = 1.0; rangeVerts[2] = 0.0;
    rangeVerts[3] = (uxPos1 - 0.5) * 2.0; rangeVerts[4] = -1.0; rangeVerts[5] = 0.0;
    glVertexPointer(3, GL_FLOAT, 0, rangeVerts);
    glDrawArrays(GL_LINES, 0, 2);

    glLineWidth((last_type == "LSB")?2.0:1.0);

    rangeVerts[0] = (uxPos2 - 0.5) * 2.0; rangeVerts[1] = 1.0; rangeVerts[2] = 0.0;
    rangeVerts[3] = (uxPos2 - 0.5) * 2.0; rangeVerts[4] = -1.0; rangeVerts[5] = 0.0;
    glDrawArrays(GL_LINES, 0, 2);
    glDisableClientState(GL_VERTEX_ARRAY);

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

void PrimaryGLContext::setHoverAlpha(float hoverAlpha_in) {
    hoverAlpha = hoverAlpha_in;
}
