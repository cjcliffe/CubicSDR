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
#include "AppFrame.h"
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

    glGenTextures(1, &waterfall);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, waterfall);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    grad.addColor(GradientColor(0, 0, 0));
    grad.addColor(GradientColor(0, 0, 1.0));
    grad.addColor(GradientColor(0, 1.0, 0));
    grad.addColor(GradientColor(1.0, 1.0, 0));
    grad.addColor(GradientColor(1.0, 0.2, 0.0));

    grad.generate(256);

    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, &(grad.getRed())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, &(grad.getGreen())[0]);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, &(grad.getBlue())[0]);

    CheckGLError();
}

void PrimaryGLContext::Plot(std::vector<float> &points, std::vector<float> &points2) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (points.size()) {
        memmove(waterfall_tex + FFT_SIZE, waterfall_tex, (NUM_WATERFALL_LINES - 1) * FFT_SIZE);

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            float v = points[i*2+1];

            float wv = v;
            if (wv<0.0) wv = 0.0;
            if (wv>1.0) wv = 1.0;
            waterfall_tex[i] = (unsigned char) floor(wv * 255.0);
        }
    }
    
    glBindTexture(GL_TEXTURE_2D, waterfall);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FFT_SIZE, NUM_WATERFALL_LINES, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *) waterfall_tex);

    glDisable(GL_TEXTURE_2D);

    glColor3f(1.0,1.0,1.0);

    if (points.size()) {
        glPushMatrix();
        glTranslatef(-1.0f, -0.9f, 0.0f);
        glScalef(2.0f, 1.0f, 1.0f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points[0]);
        glDrawArrays(GL_LINE_STRIP, 0, points.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
    }

    if (points2.size()) {
        glPushMatrix();
        glTranslatef(-1.0f, 0.5f, 0.0f);
        glScalef(2.0f, 1.0f, 1.0f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points2[0]);
        glDrawArrays(GL_LINE_STRIP, 0, points2.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
    }


    glEnable(GL_TEXTURE_2D);
    // glEnable(GL_COLOR_TABLE);
    glBindTexture(GL_TEXTURE_2D, waterfall);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-1.0, -1.0, 0.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(1.0, -1.0, 0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(1.0, 1.0, 0.0);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-1.0, 1.0, 0.0);
    glEnd();

    glFlush();

    CheckGLError();
}

wxBEGIN_EVENT_TABLE(TestGLCanvas, wxGLCanvas) EVT_PAINT(TestGLCanvas::OnPaint)
EVT_KEY_DOWN(TestGLCanvas::OnKeyDown)
EVT_IDLE(TestGLCanvas::OnIdle)
wxEND_EVENT_TABLE()

TestGLCanvas::TestGLCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), parent(parent) {

    int in_block_size = BUF_SIZE / 2;
    int out_block_size = FFT_SIZE;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * in_block_size);
    out[0] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * out_block_size);
    out[1] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * out_block_size);
    plan[0] = fftw_plan_dft_1d(out_block_size, in, out[0], FFTW_FORWARD, FFTW_MEASURE);
    plan[1] = fftw_plan_dft_1d(out_block_size, out[0], out[1], FFTW_BACKWARD, FFTW_MEASURE);

    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;
}

TestGLCanvas::~TestGLCanvas() {

}

void TestGLCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    PrimaryGLContext& canvas = wxGetApp().GetContext(this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    canvas.Plot(spectrum_points, test_demod.waveform_points);

    SwapBuffers();
}

void TestGLCanvas::OnKeyDown(wxKeyEvent& event) {
    float angle = 5.0;

    unsigned int freq;
    switch (event.GetKeyCode()) {
    case WXK_RIGHT:
        freq = ((AppFrame*) parent)->getFrequency();
        freq += 100000;
        ((AppFrame*) parent)->setFrequency(freq);
        break;
    case WXK_LEFT:
        freq = ((AppFrame*) parent)->getFrequency();
        freq -= 100000;
        ((AppFrame*) parent)->setFrequency(freq);
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

void multiply2(float ar, float aj, float br, float bj, float *cr, float *cj) {
    *cr = ar * br - aj * bj;
    *cj = aj * br + ar * bj;
}
float polar_discriminant2(float ar, float aj, float br, float bj) {
    float cr, cj;
    double angle;
    multiply2(ar, aj, br, -bj, &cr, &cj);
    angle = atan2(cj, cr);
    return (angle / M_PI);
}

void TestGLCanvas::setData(std::vector<signed char> *data) {

    if (data && data->size()) {
        if (spectrum_points.size() < FFT_SIZE * 2) {
            spectrum_points.resize(FFT_SIZE * 2);
        }

        for (int i = 0; i < BUF_SIZE / 2; i++) {
            in[i][0] = (float) (*data)[i * 2] / 127.0f;
            in[i][1] = (float) (*data)[i * 2 + 1] / 127.0f;
        }

        fftw_execute(plan[0]);

        double fft_ceil = 0, fft_floor=1;

        if (fft_result.size() < FFT_SIZE) {
            fft_result.resize(FFT_SIZE);
            fft_result_ma.resize(FFT_SIZE);
            fft_result_maa.resize(FFT_SIZE);
        }

        for (int j = 0; j < 2; j++) {
            for (int i = 0, iMax = FFT_SIZE / 2; i < iMax; i++) {
                double a = out[0][i][0];
                double b = out[0][i][1];
                double c = sqrt(a * a + b * b);

                double x = out[0][FFT_SIZE / 2 + i][0];
                double y = out[0][FFT_SIZE / 2 + i][1];
                double z = sqrt(x * x + y * y);

                fft_result[i] = (z);
                fft_result[FFT_SIZE / 2 + i] = (c);
            }
        }

        float time_slice = (float) SRATE / (float) (BUF_SIZE / 2);

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * 0.65;
            fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * 0.65;

            if (fft_result_maa[i] > fft_ceil) {
                fft_ceil = fft_result_maa[i];
            }
            if (fft_result_maa[i] < fft_floor) {
                fft_floor = fft_result_maa[i];
            }
        }

        fft_ceil += 1;
        fft_floor -= 1;
        
        fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.01;
        fft_ceil_maa = fft_ceil_maa + (fft_ceil_ma - fft_ceil_maa) * 0.01;
        
        fft_floor_ma = fft_floor_ma + (fft_floor - fft_floor_ma) * 0.01;
        fft_floor_maa = fft_floor_maa + (fft_floor_ma - fft_floor_maa) * 0.01;

        // fftw_execute(plan[1]);

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            float v = (log10(fft_result_maa[i]-fft_floor_maa) / log10(fft_ceil_maa-fft_floor_maa));
            spectrum_points[i * 2] = ((float) i / (float) iMax);
            spectrum_points[i * 2 + 1] = v;
        }

        test_demod.writeBuffer(data);
    }
}

void TestGLCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

