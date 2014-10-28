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

    new AppFrame();

    m_pThread = new SDRThread(this);
    if (m_pThread->Run() != wxTHREAD_NO_ERROR) {
        wxLogError
        ("Can't create the thread!");
        delete m_pThread;
        m_pThread = NULL;
    }

    return true;
}

int CubicSDR::OnExit() {
    delete m_glContext;

    {
        wxCriticalSectionLocker enter(m_pThreadCS);
        if (m_pThread) {
            wxMessageOutputDebug().Printf("CubicSDR: deleting thread");
            if (m_pThread->Delete() != wxTHREAD_NO_ERROR) {
                wxLogError
                ("Can't delete the thread!");
            }
        }
    }

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

