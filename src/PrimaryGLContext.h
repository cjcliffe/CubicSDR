#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

class PrimaryGLContext : public wxGLContext
{
public:
    PrimaryGLContext(wxGLCanvas *canvas);

    void PlotIQ();

private:
    // textures for the cube faces
    GLuint m_textures[6];
};

class TestGLCanvas : public wxGLCanvas
{
public:
    TestGLCanvas(wxWindow *parent, int *attribList = NULL);

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    wxDECLARE_EVENT_TABLE();
};
