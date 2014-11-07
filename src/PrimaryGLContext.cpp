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
#include "Demodulate.h"
#include "liquid.h"
#include "complex.h"

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

void PrimaryGLContext::Plot(std::vector<float> &points, std::vector<float> &points2) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

//    glEnable(GL_LINE_SMOOTH);

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
        glScalef(2.0f, 0.5f, 1.0f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &points2[0]);
        glDrawArrays(GL_LINE_STRIP, 0, points2.size() / 2);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
    }

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

    fft_ceil_ma = fft_ceil_maa = 1.0;

    dev = alcOpenDevice(NULL);
    if (!dev) {
        fprintf(stderr, "Oops\n");
    }
    ctx = alcCreateContext(dev, NULL);
    alcMakeContextCurrent(ctx);
    if (!ctx) {
        fprintf(stderr, "Oops2\n");
    }

    alGenBuffers(AL_NUM_BUFFERS, buffers);
    alGenSources(1, &source);

    // prime the buffers
    int16_t buffer_init[AL_BUFFER_SIZE];

    for (int i = 0; i < AL_BUFFER_SIZE; i++) {
        buffer_init[i] = 0;
    }

    format = AL_FORMAT_MONO16;
    for (int i = 0; i < AL_NUM_BUFFERS; i++) {
        alBufferData(buffers[i], format, buffer_init, AL_BUFFER_SIZE, demod.output.rate);
    }
    if (alGetError() != AL_NO_ERROR) {
        std::cout << "Error priming :(\n";
    }

    alSourceQueueBuffers(source, AL_NUM_BUFFERS, buffers);
    alSourcePlay(source);
    if (alGetError() != AL_NO_ERROR) {
        std::cout << "Error starting :(\n";
    }

    // define filter length, type, number of bands
    unsigned int n = 55;
    liquid_firdespm_btype btype = LIQUID_FIRDESPM_BANDPASS;
    unsigned int num_bands = 3;

    // band edge description [size: num_bands x 2]
    float bands[6] = { 0.0f, 0.14f, 0.15f, 0.35f, 0.36f, 0.5f };

    // desired response [size: num_bands x 1]
    float des[3] = { 1.0f, 0.0f, 1.0f };

    // relative weights [size: num_bands x 1]
    float weights[3] = { 1.0f, 1.0f, 1.0f };

    // in-band weighting functions [size: num_bands x 1]
    liquid_firdespm_wtype wtype[3] = { LIQUID_FIRDESPM_FLATWEIGHT, LIQUID_FIRDESPM_EXPWEIGHT, LIQUID_FIRDESPM_FLATWEIGHT };

    // allocate memory for array and design filter
    float h[n];
    firdespm_run(n, num_bands, bands, des, weights, wtype, btype, h);

    fir_filter = firfilt_crcf_create(h, n);

}

TestGLCanvas::~TestGLCanvas() {
    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(dev);

}

void TestGLCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    PrimaryGLContext& canvas = wxGetApp().GetContext(this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    canvas.Plot(spectrum_points, waveform_points);

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

void TestGLCanvas::setData(std::vector<signed char> *data) {

    if (data && data->size()) {
        /*
         std::vector<int16_t> tmp(data->begin(), data->end());
         demod.demod(tmp);

         if (waveform_points.size() < demod.lp_len * 2) {
         waveform_points.resize(demod.lp_len * 2);
         }

         float waveform_ceil = 0;

         for (int i = 0, iMax = demod.lp_len; i < iMax; i++) {
         float v = fabs(demod.lowpassed[i]);
         if (v > waveform_ceil) {
         waveform_ceil = v;
         }
         }

         for (int i = 0, iMax = demod.lp_len; i < iMax; i++) {
         waveform_points[i * 2 + 1] = (float) demod.lowpassed[i] / waveform_ceil;
         waveform_points[i * 2] = ((double) i / (double) iMax);
         }

         ALint val;
         ALuint buffer;

         alGetSourcei(source, AL_SOURCE_STATE, &val);
         if (val != AL_PLAYING) {
         alSourcePlay(source);
         }

         // std::cout << "buffer: " << demod.output_target->len << "@" << frequency << std::endl;
         std::vector<ALuint> *newBuffer = new std::vector<ALuint>;
         newBuffer->resize(demod.output_target->len);
         memcpy(&(*newBuffer)[0],demod.output_target->buf,demod.output_target->len*2);
         audio_queue.push(newBuffer);


         frequency = demod.output.rate;

         while (audio_queue.size()>8) {
         alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);
         if (val <= 0) {
         break;
         }

         std::vector<ALuint> *nextBuffer = audio_queue.front();

         alSourceUnqueueBuffers(source, 1, &buffer);
         alBufferData(buffer, format, &(*nextBuffer)[0], nextBuffer->size()*2, frequency);
         alSourceQueueBuffers(source, 1, &buffer);

         audio_queue.pop();

         delete nextBuffer;

         if (alGetError() != AL_NO_ERROR) {
         std::cout << "Error buffering :(\n";
         }
         }
         */

        if (spectrum_points.size() < FFT_SIZE * 2) {
            spectrum_points.resize(FFT_SIZE * 2);
        }

        for (int i = 0; i < BUF_SIZE / 2; i++) {

            liquid_float_complex x;
            liquid_float_complex y;

            x.real = (float) (*data)[i * 2] / 127.0f;
            x.imag = (float) (*data)[i * 2 + 1] / 127.0f;

            firfilt_crcf_push(fir_filter, x);      // push input sample
            firfilt_crcf_execute(fir_filter, &y);   // compute output

            in[i][0] = y.real;
            in[i][1] = y.imag;
        }

        fftw_execute(plan[0]);

        double fft_ceil = 0;
        // fft_floor, 

        if (fft_result.size() < FFT_SIZE) {
            fft_result.resize(FFT_SIZE);
            fft_result_ma.resize(FFT_SIZE);
            fft_result_maa.resize(FFT_SIZE);
        }

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            if (i>FFT_SIZE/4 && i < FFT_SIZE-FFT_SIZE/4) {
                out[0][i][0] = 0;
                out[0][i][1] = 0;
            }
        }

        for (int j = 0; j < 2; j++) {
            for (int i = 0, iMax = FFT_SIZE / 2; i < iMax; i++) {
                double a = out[0][i][0];
                double b = out[0][i][1];
                double c = sqrt(a * a + b * b);

                double x = out[0][FFT_SIZE / 2 + i][0];
                double y = out[0][FFT_SIZE / 2 + i][1];
                double z = sqrt(x * x + y * y);

                fft_result[i] = z;
                fft_result[FFT_SIZE/2 + i] = c;
            }
        }

        float time_slice = (float) SRATE / (float) (BUF_SIZE / 2);

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * 0.65;
            fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * 0.65;

            if (fft_result_maa[i] > fft_ceil) {
                fft_ceil = fft_result_maa[i];
            }
        }

        fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.05;
        fft_ceil_maa = fft_ceil_maa + (fft_ceil - fft_ceil_maa) * 0.05;


        fftw_execute(plan[1]);

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            spectrum_points[i * 2 + 1] = fft_result_maa[i] / fft_ceil_maa;
            spectrum_points[i * 2] = ((double) i / (double) iMax);
        }
    }
}

void TestGLCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

