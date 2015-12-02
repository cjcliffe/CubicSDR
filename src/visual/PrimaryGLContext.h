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
    PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext);

    static wxString glGetwxString(GLenum name);
    static void CheckGLError();

    void BeginDraw(float r, float g, float b);
    void EndDraw();

    void DrawFreqSelector(float uxPos, RGBA4f color, float w = 0, long long center_freq = -1, long long srate = 0);
    void DrawRangeSelector(float uxPos1, float uxPos2, RGBA4f color);
    void DrawDemod(DemodulatorInstance *demod, RGBA4f color, long long center_freq = -1, long long srate = 0);
    void DrawDemodInfo(DemodulatorInstance *demod, RGBA4f color, long long center_freq = -1, long long srate = 0);

    void setHoverAlpha(float hoverAlpha);

private:
    float hoverAlpha;
};
