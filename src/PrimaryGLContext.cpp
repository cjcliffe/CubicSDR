#include "PrimaryGLContext.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include <algorithm>

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

static void CheckGLError() {
    GLenum errLast = GL_NO_ERROR;

    for (;;) {
        GLenum err = glGetError();
        if (err == GL_NO_ERROR)
            return;

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

PrimaryGLContext::PrimaryGLContext(wxGLCanvas *canvas) :
        wxGLContext(canvas) {
    SetCurrent(*canvas);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    CheckGLError();
}

void PrimaryGLContext::PlotIQ(std::vector<float> &i_points, std::vector<float> &q_points) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.0f);
    if (q_points.size()) {
//        glScalef(10.0f, 1.0f, 1.0f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &q_points[0]);
        glDrawArrays(GL_LINE_STRIP, 0, q_points.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
    } else {
        glBegin(GL_LINE_STRIP);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);
        glEnd();
    }
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.5f, 0.0f);
    if (i_points.size()) {
//        glScalef(10.0f, 1.0f, 1.0f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &i_points[0]);
        glDrawArrays(GL_LINE_STRIP, 0, i_points.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
    } else {
        glBegin(GL_LINE_STRIP);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);
        glEnd();
    }
    glPopMatrix();

    glFlush();

    CheckGLError();
}

wxBEGIN_EVENT_TABLE(TestGLCanvas, wxGLCanvas) EVT_PAINT(TestGLCanvas::OnPaint)
EVT_KEY_DOWN(TestGLCanvas::OnKeyDown)
EVT_IDLE(TestGLCanvas::OnIdle)
wxEND_EVENT_TABLE()

TestGLCanvas::TestGLCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE) {

    int in_block_size = BUF_SIZE/2;
    int out_block_size = FFT_SIZE;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * in_block_size);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * out_block_size);
    plan = fftw_plan_dft_1d(out_block_size, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

}

void TestGLCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    PrimaryGLContext& canvas = wxGetApp().GetContext(this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    canvas.PlotIQ(i_points, q_points);

    SwapBuffers();
}

void TestGLCanvas::OnKeyDown(wxKeyEvent& event) {
    float angle = 5.0;

    switch (event.GetKeyCode()) {
    case WXK_RIGHT:
        break;
    case WXK_LEFT:
        break;
    case WXK_DOWN:
        break;
    case WXK_UP:
        break;
    case WXK_SPACE:
        break;
    default:
        event.Skip();
        return;
    }
}

void TestGLCanvas::setData(std::vector<signed char> *data) {


    if (data && data->size()) {

        if (i_points.size() < FFT_SIZE*2) {
            i_points.resize(FFT_SIZE*2);
        }

        for (int i = 0; i < BUF_SIZE / 2; i++) {
            in[i][0] = (float) (*data)[i * 2] / 127.0f;
            in[i][1] = (float) (*data)[i * 2 + 1] / 127.0f;
        }

//        for (int i = 0; i < BUF_SIZE / 2; i++) {
//            double ang = (M_PI / (float) BUF_SIZE) * (float) i;
//            double w = 0.5 * (1.0 - cos(ang));
//
//            in[i][0] *= w;
//            in[i][1] *= w;
//        }

        fftw_execute(plan);

        float result[FFT_SIZE];
        float fft_floor, fft_ceil;

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            double a = out[i][0];
            double b = out[i][1];

            double c = sqrt(a*a+b*b);
            if (i==1) {
                fft_floor=fft_ceil=c;
            } else {
                if (c<fft_floor) {
                    fft_floor = c;
                }
                if (c>fft_ceil) {
                    fft_ceil = c;
                }
            }

            result[i] = c;
        }

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            i_points[i * 2 + 1] = (result[i]-fft_floor)/(fft_ceil-fft_floor);
            i_points[i * 2] = 2.0f * ((float) i / (float) iMax) - 1.0f;
        }

    }
}

void TestGLCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

