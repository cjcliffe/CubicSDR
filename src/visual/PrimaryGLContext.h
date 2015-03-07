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
    enum GLFontSize {
        GLFONT_SIZE12, GLFONT_SIZE16, GLFONT_SIZE18, GLFONT_SIZE24, GLFONT_SIZE32, GLFONT_SIZE48, GLFONT_MAX
    };
    PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext);

    static wxString glGetwxString(GLenum name);
    static void CheckGLError();

    void BeginDraw(float r, float g, float b);
    void EndDraw();

    void DrawFreqSelector(float uxPos, RGBColor color, float w = 0, long long center_freq = -1, long long srate = 0);
    void DrawRangeSelector(float uxPos1, float uxPos2, RGBColor color);
    void DrawDemod(DemodulatorInstance *demod, RGBColor color, long long center_freq = -1, long long srate = 0);
    void DrawDemodInfo(DemodulatorInstance *demod, RGBColor color, long long center_freq = -1, long long srate = 0);

    static GLFont &getFont(GLFontSize esize);

    void setHoverAlpha(float hoverAlpha);

private:
    static GLFont fonts[GLFONT_MAX];
    DemodulatorThreadParameters defaultDemodParams;
    float hoverAlpha;
};
