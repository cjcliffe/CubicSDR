#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "CubicSDRDefs.h"
#include "GLFont.h"

class PrimaryGLContext: public wxGLContext {
public:
    PrimaryGLContext(wxGLCanvas *canvas, wxGLContext *sharedContext);

    static wxString glGetwxString(GLenum name);
    static void CheckGLError();

    static GLFont *getFont();

private:
    static GLFont *font;
};
