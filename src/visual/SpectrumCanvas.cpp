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
#include "WaterfallCanvas.h"

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
        wxFULL_REPAINT_ON_RESIZE), parent(parent), fft_size(0), in(NULL), out(NULL), plan(NULL), waterfallCanvas(NULL), center_freq(0), bandwidth(0), isView(
                0) {

    glContext = new SpectrumContext(this, &wxGetApp().GetContext(this));

    mTracker.setTarget(this);
    mTracker.setVertDragLock(true);

    SetCursor(wxCURSOR_SIZEWE);
}

void SpectrumCanvas::Setup(int fft_size_in) {
    if (fft_size == fft_size_in) {
        return;
    }

    fft_size = fft_size_in;

    if (in) {
        free(in);
    }
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_size);
    if (out) {
        free(out);
    }
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_size);
    if (plan) {
        fftw_destroy_plan(plan);
    }
    plan = fftw_plan_dft_1d(fft_size, in, out, FFTW_FORWARD, FFTW_MEASURE);

    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;
}

SpectrumCanvas::~SpectrumCanvas() {

}

void SpectrumCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->BeginDraw();
    glContext->Draw(spectrum_points, GetCenterFrequency(), GetBandwidth());

    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        glContext->DrawDemodInfo(demods[i], 1, 1, 1, GetCenterFrequency(), GetBandwidth());
    }

    glContext->EndDraw();

    SwapBuffers();
}

void SpectrumCanvas::setData(DemodulatorThreadIQData *input) {
    if (!input) {
        return;
    }
    std::vector<liquid_float_complex> *data = &input->data;
    if (data && data->size()) {
        if (fft_size != data->size()) {
            Setup(data->size());
        }
        if (spectrum_points.size() < fft_size * 2) {
            if (spectrum_points.capacity() < fft_size * 2) {
                spectrum_points.reserve(fft_size * 2);
            }
            spectrum_points.resize(fft_size * 2);
        }

        for (int i = 0; i < fft_size; i++) {
            in[i][0] = (*data)[i].real;
            in[i][1] = (*data)[i].imag;
        }

        fftw_execute(plan);

        double fft_ceil = 0, fft_floor = 1;

        if (fft_result.size() != fft_size) {
            if (fft_result.capacity() < fft_size) {
                fft_result.reserve(fft_size);
                fft_result_ma.reserve(fft_size);
                fft_result_maa.reserve(fft_size);
            }
            fft_result.resize(fft_size);
            fft_result_ma.resize(fft_size);
            fft_result_maa.resize(fft_size);
        }

        int n;
        for (int i = 0, iMax = fft_size / 2; i < iMax; i++) {
            n = (i == 0) ? 1 : i;
            double a = out[n][0];
            double b = out[n][1];
            double c = sqrt(a * a + b * b);

//            n = (i == FFT_SIZE / 2) ? (FFT_SIZE / 2 + 1) : i;
            double x = out[fft_size / 2 + n][0];
            double y = out[fft_size / 2 + n][1];
            double z = sqrt(x * x + y * y);

            fft_result[i] = (z);
            fft_result[fft_size / 2 + i] = (c);
        }

        for (int i = 0, iMax = fft_size; i < iMax; i++) {
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

        for (int i = 0, iMax = fft_size; i < iMax; i++) {
            float v = (log10(fft_result_maa[i] - fft_floor_maa) / log10(fft_ceil_maa - fft_floor_maa));
            spectrum_points[i * 2] = ((float) i / (float) iMax);
            spectrum_points[i * 2 + 1] = v;
        }

    }
}

void SpectrumCanvas::SetView(int center_freq_in, int bandwidth_in) {
    isView = true;
    center_freq = center_freq_in;
    bandwidth = bandwidth_in;
}

void SpectrumCanvas::DisableView() {
    isView = false;
    center_freq = wxGetApp().getFrequency();
    bandwidth = SRATE;
}

void SpectrumCanvas::SetCenterFrequency(unsigned int center_freq_in) {
    center_freq = center_freq_in;
}

unsigned int SpectrumCanvas::GetCenterFrequency() {
    if (isView) {
        return center_freq;
    } else {
        return (unsigned int) wxGetApp().getFrequency();
    }
}

void SpectrumCanvas::SetBandwidth(unsigned int bandwidth_in) {
    bandwidth = bandwidth_in;
}

unsigned int SpectrumCanvas::GetBandwidth() {
    if (isView) {
        return bandwidth;
    } else {
        return SRATE;
    }
}

void SpectrumCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

void SpectrumCanvas::mouseMoved(wxMouseEvent& event) {
    mTracker.OnMouseMoved(event);
    if (mTracker.mouseDown()) {
        int freqChange = mTracker.getDeltaMouseX() * GetBandwidth();

        if (freqChange != 0) {
            int freq = wxGetApp().getFrequency();

            if (isView) {
                center_freq -= freqChange;
                if (waterfallCanvas) {
                    waterfallCanvas->SetCenterFrequency(center_freq);
                }

                if (SRATE/2 < (abs(freq-center_freq)+bandwidth/2)) {
//                    if (center_freq < freq) {
//                        freqChange = -((center_freq - (freq - bandwidth / 2)) - (SRATE/2));
//                    } else {
//                        freqChange = ((freq + bandwidth / 2) - center_freq - (SRATE/2));
//                    }
                } else {
                    freqChange = 0;
                }
            }

            if (freqChange) {
                freq -= freqChange;
                wxGetApp().setFrequency(freq);
                ((wxFrame*) parent)->GetStatusBar()->SetStatusText(
                        wxString::Format(wxT("Set center frequency: %s"),
                                wxNumberFormatter::ToString((long) freq, wxNumberFormatter::Style_WithThousandsSep)));
            }

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

void SpectrumCanvas::attachWaterfallCanvas(WaterfallCanvas* canvas_in) {
    waterfallCanvas = canvas_in;
}
//void SpectrumCanvas::rightClick(wxMouseEvent& event) {}
//void SpectrumCanvas::keyPressed(wxKeyEvent& event) {}
//void SpectrumCanvas::keyReleased(wxKeyEvent& event) {}

