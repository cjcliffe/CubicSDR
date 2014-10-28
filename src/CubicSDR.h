#pragma once

//WX_GL_CORE_PROFILE 1
//WX_GL_MAJOR_VERSION 3
//WX_GL_MINOR_VERSION 2



#include "SDRThread.h"
#include "wx/glcanvas.h"
#include "PrimaryGLContext.h"

class CubicSDR : public wxApp
{
public:
    CubicSDR() { m_glContext = NULL; m_pThread = NULL; }

    PrimaryGLContext &GetContext(wxGLCanvas *canvas);

    virtual bool OnInit();
    virtual int OnExit();

private:
    PrimaryGLContext *m_glContext;
    SDRThread *m_pThread;
    wxCriticalSection m_pThreadCS;
};

DECLARE_APP(CubicSDR)
