#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include "CubicSDRDefs.h"
#include "fftw3.h"

class PrimaryGLContext : public wxGLContext
{
public:
    PrimaryGLContext(wxGLCanvas *canvas);

    void Plot(std::vector<float> &points);

private:
    // textures for the cube faces
    GLuint m_textures[6];
};

class TestGLCanvas : public wxGLCanvas
{
public:
    TestGLCanvas(wxWindow *parent, int *attribList = NULL);

    void setData(std::vector<signed char> *data);

    std::vector<float> points;

    fftw_complex *in, *out[2];
    fftw_plan plan[2];

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

    wxDECLARE_EVENT_TABLE();
};
