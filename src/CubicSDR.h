#pragma once

//WX_GL_CORE_PROFILE 1
//WX_GL_MAJOR_VERSION 3
//WX_GL_MINOR_VERSION 2

#include "wx/glcanvas.h"
#include "SDRThread.h"

// the rendering context used by all GL canvases
class PrimaryGLContext : public wxGLContext
{
public:
    PrimaryGLContext(wxGLCanvas *canvas);

    // render the cube showing it at given angles
    void DrawRotatedCube(float xangle, float yangle);

private:
    // textures for the cube faces
    GLuint m_textures[6];
};


class CubicSDR : public wxApp
{
public:
    CubicSDR() { m_glContext = NULL; }

    PrimaryGLContext& GetContext(wxGLCanvas *canvas);

    virtual bool OnInit();
    virtual int OnExit();

private:
    PrimaryGLContext *m_glContext;
    SDRThread *m_pThread;
    wxCriticalSection m_pThreadCS;
};

// Define a new frame type
class AppFrame : public wxFrame
{
public:
    AppFrame();

private:
    void OnClose(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

class TestGLCanvas : public wxGLCanvas
{
public:
    TestGLCanvas(wxWindow *parent, int *attribList = NULL);

private:
    void OnPaint(wxPaintEvent& event);
    void Spin(float xSpin, float ySpin);
    void OnKeyDown(wxKeyEvent& event);
    void OnSpinTimer(wxTimerEvent& WXUNUSED(event));

    // angles of rotation around x- and y- axis
    float m_xangle,
          m_yangle;

    wxTimer m_spinTimer;

    wxDECLARE_EVENT_TABLE();
};

