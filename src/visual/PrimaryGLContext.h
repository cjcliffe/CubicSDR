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

    bool SetCurrent(const wxGLCanvas& win) const override;

private:
    mutable bool swapIntervalApplied = false;
};
