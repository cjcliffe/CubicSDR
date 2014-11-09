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

#include "pa_debugprint.h"

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
        glScalef(2.0f, 1.0f, 1.0f);
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

static int patestCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData) {

    TestGLCanvas *src = (TestGLCanvas *) userData;

    if (!src->audio_queue.size()) {
        return paContinue;
    }
    float *out = (float*) outputBuffer;

    std::vector<float> *nextBuffer = src->audio_queue.front();

    for (int i = 0; i < framesPerBuffer * 2; i++) {
        out[i] = (*nextBuffer)[src->audio_queue_ptr];

        src->audio_queue_ptr++;

        if (src->audio_queue_ptr == nextBuffer->size()) {
            src->audio_queue.pop();
            delete nextBuffer;
            src->audio_queue_ptr = 0;
            if (!src->audio_queue.size()) {
                break;
            }
            nextBuffer = src->audio_queue.front();
        }
    }

    return paContinue;
}

TestGLCanvas::TestGLCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), parent(parent) {

    bandwidth = 800000;
    resample_ratio = (float) (bandwidth) / (float) SRATE;
    audio_frequency = 44100;
    audio_resample_ratio = (float) (audio_frequency) / (float) bandwidth;

    int in_block_size = BUF_SIZE / 2;
    int out_block_size = FFT_SIZE;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * in_block_size);
    out[0] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * out_block_size);
    out[1] = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * out_block_size);
    plan[0] = fftw_plan_dft_1d(out_block_size, in, out[0], FFTW_FORWARD, FFTW_MEASURE);
    plan[1] = fftw_plan_dft_1d(out_block_size, out[0], out[1], FFTW_BACKWARD, FFTW_MEASURE);

    fft_ceil_ma = fft_ceil_maa = 1.0;

    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cout << "Error starting :(\n";
    }

    outputParameters.device = 5; /* default output device */
    if (outputParameters.device == paNoDevice) {
        std::cout << "Error: No default output device.\n";
    }

    outputParameters.channelCount = 2; /* Stereo output, most likely supported. */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output. */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    stream = NULL;

    err = Pa_OpenStream(&stream, NULL, &outputParameters, 44100, 256, paClipOff, &patestCallback, this);

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cout << "Error starting stream: " << Pa_GetErrorText(err) << std::endl;
        std::cout << "\tPortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    float fc = 0.5f * (bandwidth / SRATE);         // filter cutoff frequency
    float ft = 0.05f;         // filter transition
    float As = 60.0f;         // stop-band attenuation [dB]
    float mu = 0.0f;          // fractional timing offset

    // estimate required filter length and generate filter
    unsigned int h_len = estimate_req_filter_len(ft, As);
    float h[h_len];
    liquid_firdes_kaiser(h_len, fc, As, mu, h);

    fir_filter = firfilt_crcf_create(h, h_len);

    // create multi-stage arbitrary resampler object
    resampler = msresamp_crcf_create(resample_ratio, As);
    msresamp_crcf_print(resampler);

    audio_resampler = msresamp_crcf_create(audio_resample_ratio, As);
    msresamp_crcf_print(audio_resampler);

    float kf = 0.1f;        // modulation factor

    fdem = freqdem_create(kf);
    freqdem_print(fdem);
}

TestGLCanvas::~TestGLCanvas() {
    PaError err;
    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);
    Pa_Terminate();
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

        fftw_execute(plan[0]);

        liquid_float_complex filtered_input[BUF_SIZE / 2];

        for (int i = 0; i < BUF_SIZE / 2; i++) {

            liquid_float_complex x;
            liquid_float_complex y;

            x.real = (float) (*data)[i * 2] / 127.0f;
            x.imag = (float) (*data)[i * 2 + 1] / 127.0f;

            firfilt_crcf_push(fir_filter, x);      // push input sample
            firfilt_crcf_execute(fir_filter, &y);   // compute output

            filtered_input[i] = y;
            in[i][0] = x.real;
            in[i][1] = x.imag;
        }

        int out_size = ceil((float) (BUF_SIZE / 2) * resample_ratio);

        liquid_float_complex resampled_output[out_size];

        unsigned int num_written;       // number of values written to buffer
        msresamp_crcf_execute(resampler, filtered_input, (BUF_SIZE / 2), resampled_output, &num_written);

        double fft_ceil = 0;
        // fft_floor, 

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
        }

        fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.05;
        fft_ceil_maa = fft_ceil_maa + (fft_ceil - fft_ceil_maa) * 0.05;

        // fftw_execute(plan[1]);

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            spectrum_points[i * 2 + 1] = log10(fft_result_maa[i]) / log10(fft_ceil_maa);
//            spectrum_points[i * 2 + 1] = (fft_result_maa[i]) / (fft_ceil_maa);
            spectrum_points[i * 2] = ((double) i / (double) iMax);
        }

        float waveform_ceil = 0, waveform_floor = 0;

        float pcm = 0;

        for (int i = 0; i < num_written; i++) {
            freqdem_demodulate(fdem, resampled_output[i], &pcm);

            resampled_output[i].real = (float) pcm;
            resampled_output[i].imag = 0;

            if (waveform_ceil < resampled_output[i].real) {
                waveform_ceil = resampled_output[i].real;
            }

            if (waveform_floor > resampled_output[i].real) {
                waveform_floor = resampled_output[i].real;
            }
        }

        int audio_out_size = ceil((float) (num_written) * audio_resample_ratio);
        liquid_float_complex resampled_audio_output[audio_out_size];

        unsigned int num_audio_written;       // number of values written to buffer
        msresamp_crcf_execute(audio_resampler, resampled_output, num_written, resampled_audio_output, &num_audio_written);

        if (waveform_points.size() != num_audio_written * 2) {
            waveform_points.resize(num_audio_written * 2);
        }

        for (int i = 0, iMax = waveform_points.size() / 2; i < iMax; i++) {
            waveform_points[i * 2 + 1] = resampled_audio_output[i].real * 0.5f;
            waveform_points[i * 2] = ((double) i / (double) iMax);
        }

        std::vector<float> *newBuffer = new std::vector<float>;
        newBuffer->resize(num_audio_written * 2);
        for (int i = 0; i < num_audio_written; i++) {
            (*newBuffer)[i * 2] = resampled_audio_output[i].real;
            (*newBuffer)[i * 2 + 1] = resampled_audio_output[i].real;
        }

        audio_queue.push(newBuffer);
    }
}

void TestGLCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

