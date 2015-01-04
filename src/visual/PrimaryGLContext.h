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
    enum GLFontSize {
        GLFONT_SIZE12, GLFONT_SIZE16, GLFONT_SIZE18, GLFONT_SIZE24, GLFONT_SIZE32, GLFONT_SIZE48, GLFONT_MAX
    };
    PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext);

    static wxString glGetwxString(GLenum name);
    static void CheckGLError();

    void BeginDraw();
    void EndDraw();

    void DrawFreqSelector(float uxPos, float r = 1, float g = 1, float b = 1, float w = 0, long long center_freq = -1, long long srate = SRATE);
    void DrawDemod(DemodulatorInstance *demod, float r = 1, float g = 1, float b = 1, long long center_freq = -1, long long srate = SRATE);
    void DrawDemodInfo(DemodulatorInstance *demod, float r = 1, float g = 1, float b = 1, long long center_freq = -1, long long srate = SRATE);

    static GLFont &getFont(GLFontSize esize);

private:
    static GLFont fonts[GLFONT_MAX];
    DemodulatorThreadParameters defaultDemodParams;
};
