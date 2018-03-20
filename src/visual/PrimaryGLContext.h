// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "CubicSDRDefs.h"
#include "GLFont.h"
#include "DemodulatorMgr.h"
#include "ColorTheme.h"

class PrimaryGLContext: public wxGLContext {
public:
    PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext, wxGLContextAttrs* ctxAttrs);

    static wxString glGetwxString(GLenum name);
    static void CheckGLError();

    void BeginDraw(float r, float g, float b);
    void EndDraw();

    void DrawFreqSelector(float uxPos, RGBA4f color, float w = 0, long long center_freq = -1, long long srate = 0);
    void DrawRangeSelector(float uxPos1, float uxPos2, RGBA4f color);
    void DrawDemod(DemodulatorInstancePtr demod, RGBA4f color, long long center_freq = -1, long long srate = 0);
    
    void DrawDemodInfo(DemodulatorInstancePtr demod, RGBA4f color, long long center_freq = -1, long long srate = 0, bool centerline = false);
    void DrawFreqBwInfo(long long freq, int bw, RGBA4f color, long long center_freq = - 1, long long srate = 0, bool stack = false, bool centerline = false);

    void setHoverAlpha(float hoverAlpha);

private:
    float hoverAlpha;
    void drawSingleDemodLabel(const std::wstring& demodStr, float uxPos, float hPos, float xOfs, float yOfs, GLFont::Align demodAlign);
};
