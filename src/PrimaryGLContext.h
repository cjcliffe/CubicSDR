#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "CubicSDRDefs.h"
#include "fftw3.h"

#include "liquid/liquid.h"

#include "Demodulate.h"

#ifdef WIN32
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#define AL_NUM_BUFFERS 16
#define AL_BUFFER_SIZE 4096

class PrimaryGLContext: public wxGLContext {
public:
    PrimaryGLContext(wxGLCanvas *canvas);

    void Plot(std::vector<float> &points, std::vector<float> &points2);

private:
    // textures for the cube faces
    GLuint m_textures[6];
};

class TestGLCanvas: public wxGLCanvas {
public:
    TestGLCanvas(wxWindow *parent, int *attribList = NULL);
    ~TestGLCanvas();

    void setData(std::vector<signed char> *data);

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

    wxWindow *parent;
    std::vector<float> spectrum_points;
    std::vector<float> waveform_points;

    fftw_complex *in, *out[2];
    fftw_plan plan[2];

    firfilt_crcf fir_filter;

    firhilbf fir_hil;

    float fft_ceil_ma, fft_ceil_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    std::queue< std::vector <ALuint> * > audio_queue;

    Demodulate demod;

    ALCdevice *dev;
    ALCcontext *ctx;

    ALuint source, buffers[AL_NUM_BUFFERS];
    ALuint frequency;
    ALenum format;

wxDECLARE_EVENT_TABLE();
};
