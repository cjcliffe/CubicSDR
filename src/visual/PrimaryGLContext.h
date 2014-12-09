#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "CubicSDRDefs.h"
#include "GLFont.h"
#include "DemodulatorMgr.h"

class PrimaryGLContext: public wxGLContext {
public:
    PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext);

    static wxString glGetwxString(GLenum name);
    static void CheckGLError();

    void BeginDraw();
    void EndDraw();

    void DrawFreqSelector(float uxPos, float r = 1, float g = 1, float b = 1);
    void DrawDemod(DemodulatorInstance *demod, float r = 1, float g = 1, float b = 1);

    static GLFont *getFont();

private:
    static GLFont *font;
};
