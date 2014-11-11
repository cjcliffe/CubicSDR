#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "CubicSDRDefs.h"
#include "fftw3.h"
#include "Demodulator.h"

#include "Gradient.h"

#define NUM_WATERFALL_LINES 256

class PrimaryGLContext: public wxGLContext {
public:
    PrimaryGLContext(wxGLCanvas *canvas);

    void Plot(std::vector<float> &points, std::vector<float> &points2, GLuint tex);

private:
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

    fftw_complex *in, *out[2];
    fftw_plan plan[2];

    float fft_ceil_ma, fft_ceil_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    Gradient grad;
    
    std::vector<unsigned char> color_map;

    unsigned char waterfall_tex[FFT_SIZE * NUM_WATERFALL_LINES];
  
    GLuint waterfall;
  
    Demodulator test_demod;
wxDECLARE_EVENT_TABLE();
};
