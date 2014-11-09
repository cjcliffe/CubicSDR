#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "CubicSDRDefs.h"
#include "fftw3.h"

#include "liquid/liquid.h"

#include "portaudio.h"
#include "pa_stream.h"

class PrimaryGLContext: public wxGLContext {
public:
    PrimaryGLContext(wxGLCanvas *canvas);

    void Plot(std::vector<float> &points, std::vector<float> &points2);

private:
};

class TestGLCanvas: public wxGLCanvas {
public:
    TestGLCanvas(wxWindow *parent, int *attribList = NULL);
    ~TestGLCanvas();

    void setData(std::vector<signed char> *data);

    std::queue< std::vector <float> * > audio_queue;
    unsigned int audio_queue_ptr;

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

    float pre_r;
    float pre_j;
    float droop_ofs, droop_ofs_ma, droop_ofs_maa;

    msresamp_crcf resampler;
    msresamp_crcf audio_resampler;
    float resample_ratio;

    freqdem fdem;

    float fft_ceil_ma, fft_ceil_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    unsigned int bandwidth;
    unsigned int audio_frequency;
    float audio_resample_ratio;

    PaStreamParameters outputParameters;
    PaStream *stream;


wxDECLARE_EVENT_TABLE();
};
