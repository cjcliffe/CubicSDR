#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>

class PrimaryGLContext : public wxGLContext
{
public:
    PrimaryGLContext(wxGLCanvas *canvas);

    void PlotIQ(std::vector<float> &i_points, std::vector<float> &q_points);

private:
    // textures for the cube faces
    GLuint m_textures[6];
};

class TestGLCanvas : public wxGLCanvas
{
public:
    TestGLCanvas(wxWindow *parent, int *attribList = NULL);

    void setData(std::vector<signed char> *data);

    std::vector<float> i_points;
    std::vector<float> q_points;

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

    wxDECLARE_EVENT_TABLE();
};
