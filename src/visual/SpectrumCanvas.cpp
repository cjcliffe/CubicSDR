#include "SpectrumCanvas.h"

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
#include <wx/numformatter.h>

wxBEGIN_EVENT_TABLE(SpectrumCanvas, wxGLCanvas) EVT_PAINT(SpectrumCanvas::OnPaint)
EVT_IDLE(SpectrumCanvas::OnIdle)
EVT_MOTION(SpectrumCanvas::mouseMoved)
EVT_LEFT_DOWN(SpectrumCanvas::mouseDown)
EVT_LEFT_UP(SpectrumCanvas::mouseReleased)
EVT_LEAVE_WINDOW(SpectrumCanvas::mouseLeftWindow)
EVT_MOUSEWHEEL(SpectrumCanvas::mouseWheelMoved)
wxEND_EVENT_TABLE()

SpectrumCanvas::SpectrumCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), parent(parent), frameTimer(0) {

    int in_block_size = FFT_SIZE;
    int out_block_size = FFT_SIZE;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * in_block_size);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * out_block_size);
    plan = fftw_plan_dft_1d(out_block_size, in, out, FFTW_FORWARD, FFTW_MEASURE);

    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;

    glContext = new SpectrumContext(this, &wxGetApp().GetContext(this));
    timer.start();

    mTracker.setTarget(this);
    mTracker.setVertDragLock(true);

    SetCursor(wxCURSOR_SIZEWE);
}

SpectrumCanvas::~SpectrumCanvas() {

}

void SpectrumCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->BeginDraw();
    glContext->Draw(spectrum_points);

    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        glContext->DrawDemodInfo(demods[i]);
    }

    glContext->EndDraw();

    SwapBuffers();
}

void SpectrumCanvas::setData(std::vector<signed char> *data) {

    if (data && data->size()) {
        if (spectrum_points.size() < FFT_SIZE * 2) {
            spectrum_points.resize(FFT_SIZE * 2);
        }

        for (int i = 0; i < FFT_SIZE; i++) {
            in[i][0] = (float) (*data)[i * 2] / 127.0f;
            in[i][1] = (float) (*data)[i * 2 + 1] / 127.0f;
        }

        fftw_execute(plan);

        double fft_ceil = 0, fft_floor = 1;

        if (fft_result.size() < FFT_SIZE) {
            fft_result.resize(FFT_SIZE);
            fft_result_ma.resize(FFT_SIZE);
            fft_result_maa.resize(FFT_SIZE);
        }

        int n;
        for (int i = 0, iMax = FFT_SIZE / 2; i < iMax; i++) {
            n = (i == 0) ? 1 : i;
            double a = out[n][0];
            double b = out[n][1];
            double c = sqrt(a * a + b * b);

            n = (i == FFT_SIZE / 2) ? (FFT_SIZE / 2 + 1) : i;
            double x = out[FFT_SIZE / 2 + n][0];
            double y = out[FFT_SIZE / 2 + n][1];
            double z = sqrt(x * x + y * y);

            fft_result[i] = (z);
            fft_result[FFT_SIZE / 2 + i] = (c);
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
            float v = (log10(fft_result_maa[i] - fft_floor_maa) / log10(fft_ceil_maa - fft_floor_maa));
            spectrum_points[i * 2] = ((float) i / (float) iMax);
            spectrum_points[i * 2 + 1] = v;
        }

    }
}

void SpectrumCanvas::OnIdle(wxIdleEvent &event) {
//    timer.update();
//    frameTimer += timer.lastUpdateSeconds();
//    if (frameTimer > 1.0/30.0) {
    Refresh(false);
//        frameTimer = 0;
//    }
}

void SpectrumCanvas::mouseMoved(wxMouseEvent& event) {
    mTracker.OnMouseMoved(event);
    if (mTracker.mouseDown()) {
        int freqChange = mTracker.getDeltaMouseX() * SRATE;

        if (freqChange != 0) {
            int freq = wxGetApp().getFrequency();
            freq -= freqChange;
            wxGetApp().setFrequency(freq);

            ((wxFrame*) parent)->GetStatusBar()->SetStatusText(
                    wxString::Format(wxT("Set center frequency: %s"),
                            wxNumberFormatter::ToString((long) freq, wxNumberFormatter::Style_WithThousandsSep)));
        }
    }
}

void SpectrumCanvas::mouseDown(wxMouseEvent& event) {
    mTracker.OnMouseDown(event);
    SetCursor(wxCURSOR_CROSS);
}

void SpectrumCanvas::mouseWheelMoved(wxMouseEvent& event) {
    mTracker.OnMouseWheelMoved(event);
}

void SpectrumCanvas::mouseReleased(wxMouseEvent& event) {
    mTracker.OnMouseReleased(event);
    SetCursor(wxCURSOR_SIZEWE);
}

void SpectrumCanvas::mouseLeftWindow(wxMouseEvent& event) {
    mTracker.OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_SIZEWE);
}

//void SpectrumCanvas::rightClick(wxMouseEvent& event) {}
//void SpectrumCanvas::keyPressed(wxKeyEvent& event) {}
//void SpectrumCanvas::keyReleased(wxKeyEvent& event) {}

