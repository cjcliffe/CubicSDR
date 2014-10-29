#define OPENGL

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "AppFrame.h"

IMPLEMENT_APP(CubicSDR)

bool CubicSDR::OnInit() {
    if (!wxApp::OnInit())
        return false;

    AppFrame *appframe = new AppFrame();

    t_SDR = new SDRThread(appframe);
    if (t_SDR->Run() != wxTHREAD_NO_ERROR) {
        wxLogError
        ("Can't create the thread!");
        delete t_SDR;
        t_SDR = NULL;
    }

    t_IQBuffer = new IQBufferThread(this);
    if (t_IQBuffer->Run() != wxTHREAD_NO_ERROR) {
        wxLogError
        ("Can't create the thread!");
        delete t_IQBuffer;
        t_IQBuffer = NULL;
    }

    return true;
}

int CubicSDR::OnExit() {
    delete m_glContext;

    {
        wxCriticalSectionLocker enter(m_pThreadCS);
        if (t_SDR) {
            wxMessageOutputDebug().Printf("CubicSDR: deleting thread");
            if (t_SDR->Delete() != wxTHREAD_NO_ERROR) {
                wxLogError
                ("Can't delete the thread!");
            }
        }
    }

    {
        wxCriticalSectionLocker enter(m_pThreadCS);
        if (t_IQBuffer) {
            wxMessageOutputDebug().Printf("CubicSDR: deleting thread");
            if (t_IQBuffer->Delete() != wxTHREAD_NO_ERROR) {
                wxLogError
                ("Can't delete the thread!");
            }
        }
    }
    wxThread::This()->Sleep(1);

//	while (1) {
//		{ wxCriticalSectionLocker enter(m_pThreadCS);
//			if (!m_pThread)
//				break;
//		}
//		// wait for thread completion
//		wxThread::This()->Sleep(1);
//	}

    return wxApp::OnExit();
}

PrimaryGLContext& CubicSDR::GetContext(wxGLCanvas *canvas) {
    PrimaryGLContext *glContext;
    if (!m_glContext) {
        m_glContext = new PrimaryGLContext(canvas);
    }
    glContext = m_glContext;

    glContext->SetCurrent(*canvas);

    return *glContext;
}

