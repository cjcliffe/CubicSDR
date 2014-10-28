#define OPENGL

//WX_GL_CORE_PROFILE 1
//WX_GL_MAJOR_VERSION 3
//WX_GL_MINOR_VERSION 2

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum {
    SpinTimer = wxID_HIGHEST + 1
};

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

static void CheckGLError() {
    GLenum errLast = GL_NO_ERROR;

    for (;;) {
        GLenum err = glGetError();
        if (err == GL_NO_ERROR)
            return;

        // normally the error is reset by the call to glGetError() but if
        // glGetError() itself returns an error, we risk looping forever here
        // so check that we get a different error than the last time
        if (err == errLast) {
            wxLogError
            (wxT("OpenGL error state couldn't be reset."));
            return;
        }

        errLast = err;

        wxLogError
        (wxT("OpenGL error %d"), err);
    }
}

// function to draw the texture for cube faces
static wxImage DrawDice(int size, unsigned num) {
    wxASSERT_MSG(num >= 1 && num <= 6, wxT("invalid dice index"));

    const int dot = size / 16;        // radius of a single dot
    const int gap = 5 * size / 32;      // gap between dots

    wxBitmap bmp(size, size);
    wxMemoryDC dc;
    dc.SelectObject(bmp);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    dc.SetBrush(*wxBLACK_BRUSH);

    // the upper left and lower right points
    if (num != 1) {
        dc.DrawCircle(gap + dot, gap + dot, dot);
        dc.DrawCircle(size - gap - dot, size - gap - dot, dot);
    }

    // draw the central point for odd dices
    if (num % 2) {
        dc.DrawCircle(size / 2, size / 2, dot);
    }

    // the upper right and lower left points
    if (num > 3) {
        dc.DrawCircle(size - gap - dot, gap + dot, dot);
        dc.DrawCircle(gap + dot, size - gap - dot, dot);
    }

    // finally those 2 are only for the last dice
    if (num == 6) {
        dc.DrawCircle(gap + dot, size / 2, dot);
        dc.DrawCircle(size - gap - dot, size / 2, dot);
    }

    dc.SelectObject(wxNullBitmap);

    return bmp.ConvertToImage();
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// PrimaryGLContext
// ----------------------------------------------------------------------------

PrimaryGLContext::PrimaryGLContext(wxGLCanvas *canvas) :
        wxGLContext(canvas) {
    SetCurrent(*canvas);

    // set up the parameters we want to use
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_TEXTURE_2D);

    // add slightly more light, the default lighting is rather dark
    GLfloat ambient[] = { 0.5, 0.5, 0.5, 0.5 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    // set viewing projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 3.0f);

    // create the textures to use for cube sides: they will be reused by all
    // canvases (which is probably not critical in the case of simple textures
    // we use here but could be really important for a real application where
    // each texture could take many megabytes)
    glGenTextures(WXSIZEOF(m_textures), m_textures);

    for (unsigned i = 0; i < WXSIZEOF(m_textures); i++) {
        glBindTexture(GL_TEXTURE_2D, m_textures[i]);

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        const wxImage img(DrawDice(256, i + 1));

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.GetData());
    }

    CheckGLError();
}

void PrimaryGLContext::DrawRotatedCube(float xangle, float yangle) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -2.0f);
    glRotatef(xangle, 1.0f, 0.0f, 0.0f);
    glRotatef(yangle, 0.0f, 1.0f, 0.0f);

    // draw six faces of a cube of size 1 centered at (0, 0, 0)
    glBindTexture(GL_TEXTURE_2D, m_textures[0]);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0, 0);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(1, 0);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glTexCoord2f(1, 1);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(0, 1);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, m_textures[1]);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0, 0);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glTexCoord2f(1, 1);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(0, 1);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, m_textures[2]);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0, 0);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(1, 0);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glTexCoord2f(1, 1);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glTexCoord2f(0, 1);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, m_textures[3]);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(0, 0);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 1);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(0, 1);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, m_textures[4]);
    glBegin(GL_QUADS);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0, 0);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glTexCoord2f(1, 0);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glTexCoord2f(1, 1);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glTexCoord2f(0, 1);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, m_textures[5]);
    glBegin(GL_QUADS);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0, 0);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glTexCoord2f(1, 0);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glTexCoord2f(1, 1);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glTexCoord2f(0, 1);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glEnd();

    glFlush();

    CheckGLError();
}

// ----------------------------------------------------------------------------
// CubicSDR: the application object
// ----------------------------------------------------------------------------

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
        // Create the OpenGL context for the first mono window which needs it:
        // subsequently created windows will all share the same context.
        m_glContext = new PrimaryGLContext(canvas);
    }
    glContext = m_glContext;

    glContext->SetCurrent(*canvas);

    return *glContext;
}

// ----------------------------------------------------------------------------
// TestGLCanvas
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(TestGLCanvas, wxGLCanvas) EVT_PAINT(TestGLCanvas::OnPaint)
EVT_KEY_DOWN(TestGLCanvas::OnKeyDown)
EVT_TIMER(SpinTimer, TestGLCanvas::OnSpinTimer)
wxEND_EVENT_TABLE()

TestGLCanvas::TestGLCanvas(wxWindow *parent, int *attribList)
// With perspective OpenGL graphics, the wxFULL_REPAINT_ON_RESIZE style
// flag should always be set, because even making the canvas smaller should
// be followed by a paint event that updates the entire canvas with new
// viewport settings.
:
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), m_xangle(30.0), m_yangle(30.0), m_spinTimer(this, SpinTimer) {
//    if ( attribList )
//    {
//        int i = 0;
//        while ( attribList[i] != 0 )
//        {
//            ++i;
//        }
//    }
}

void TestGLCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    // This is required even though dc is not used otherwise.
    wxPaintDC dc(this);

    // Set the OpenGL viewport according to the client size of this canvas.
    // This is done here rather than in a wxSizeEvent handler because our
    // OpenGL rendering context (and thus viewport setting) is used with
    // multiple canvases: If we updated the viewport in the wxSizeEvent
    // handler, changing the size of one canvas causes a viewport setting that
    // is wrong when next another canvas is repainted.
    const wxSize ClientSize = GetClientSize();

    PrimaryGLContext& canvas = wxGetApp().GetContext(this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    // Render the graphics and swap the buffers.
    canvas.DrawRotatedCube(m_xangle, m_yangle);

    SwapBuffers();
}

void TestGLCanvas::Spin(float xSpin, float ySpin) {
    m_xangle += xSpin;
    m_yangle += ySpin;

    Refresh(false);
}

void TestGLCanvas::OnKeyDown(wxKeyEvent& event) {
    float angle = 5.0;

    switch (event.GetKeyCode()) {
    case WXK_RIGHT:
        Spin(0.0, -angle);
        break;

    case WXK_LEFT:
        Spin(0.0, angle);
        break;

    case WXK_DOWN:
        Spin(-angle, 0.0);
        break;

    case WXK_UP:
        Spin(angle, 0.0);
        break;

    case WXK_SPACE:
        if (m_spinTimer.IsRunning())
            m_spinTimer.Stop();
        else
            m_spinTimer.Start(1);
        break;

    default:
        event.Skip();
        return;
    }
}

void TestGLCanvas::OnSpinTimer(wxTimerEvent& WXUNUSED(event)) {
    Spin(0.0, 4.0);
}

wxString glGetwxString(GLenum name) {
    const GLubyte *v = glGetString(name);
    if (v == 0) {
        // The error is not important. It is GL_INVALID_ENUM.
        // We just want to clear the error stack.
        glGetError();

        return wxString();
    }

    return wxString((const char*) v);
}

// ----------------------------------------------------------------------------
// AppFrame: main application window
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame) EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_MENU(wxID_CLOSE, AppFrame::OnClose)
wxEND_EVENT_TABLE()

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, wxT("CubicSDR")) {

    new TestGLCanvas(this, NULL);

    SetIcon(wxICON(sample));

    // Make a menubar
    wxMenu *menu = new wxMenu;
    menu->Append(wxID_NEW);
    menu->AppendSeparator();
    menu->Append(wxID_CLOSE);
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menu, wxT("&Cube"));

    SetMenuBar(menuBar);

    CreateStatusBar();

    SetClientSize(400, 400);
    Show();

//    static const int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
//    wxLogStatus("Double-buffered display %s supported", wxGLCanvas::IsDisplaySupported(attribs) ? "is" : "not");
//    ShowFullScreen(true);
}

void AppFrame::OnClose(wxCommandEvent& WXUNUSED(event)) {
    // true is to force the frame to close
    Close(true);
}

void AppFrame::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
    new AppFrame();
}
