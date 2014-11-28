#include "WaterfallCanvas.h"

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

wxBEGIN_EVENT_TABLE(WaterfallCanvas, wxGLCanvas) EVT_PAINT(WaterfallCanvas::OnPaint)
EVT_KEY_DOWN(WaterfallCanvas::OnKeyDown)
EVT_IDLE(WaterfallCanvas::OnIdle)
EVT_MOTION(WaterfallCanvas::mouseMoved)
EVT_LEFT_DOWN(WaterfallCanvas::mouseDown)
EVT_LEFT_UP(WaterfallCanvas::mouseReleased)
EVT_LEAVE_WINDOW(WaterfallCanvas::mouseLeftWindow)
EVT_ENTER_WINDOW(WaterfallCanvas::mouseEnterWindow)
wxEND_EVENT_TABLE()

WaterfallCanvas::WaterfallCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), parent(parent), frameTimer(0), bwChange(false), demodBW(0) {

    int in_block_size = BUF_SIZE / 2;
    int out_block_size = FFT_SIZE;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * in_block_size);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * out_block_size);
    plan = fftw_plan_dft_1d(out_block_size, in, out, FFTW_FORWARD, FFTW_MEASURE);

    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;

    glContext = new WaterfallContext(this, &wxGetApp().GetContext(this));
    timer.start();

    mTracker.setTarget(this);
    mTracker.setVertDragLock(true);
    mTracker.setHorizDragLock(true);
    SetCursor(wxCURSOR_CROSS);
}

WaterfallCanvas::~WaterfallCanvas() {

}

void WaterfallCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->BeginDraw();
    glContext->Draw(spectrum_points);

    std::vector<DemodulatorInstance *> *demods = &wxGetApp().getDemodMgr().getDemodulators();

    for (int i = 0, iMax = demods->size(); i < iMax; i++) {
        glContext->DrawDemod((*demods)[i]);
    }

    if (mTracker.mouseInView()) {
        glContext->DrawFreqSelector(mTracker.getMouseX());
    }
    glContext->EndDraw();

    SwapBuffers();
}

void WaterfallCanvas::OnKeyDown(wxKeyEvent& event) {
    float angle = 5.0;

    unsigned int freq;
    switch (event.GetKeyCode()) {
    case WXK_RIGHT:
        freq = wxGetApp().getFrequency();
        freq += 100000;
        wxGetApp().setFrequency(freq);
        ((wxFrame*) parent)->GetStatusBar()->SetStatusText(wxString::Format(wxT("Set center frequency: %i"), freq));
        break;
    case WXK_LEFT:
        freq = wxGetApp().getFrequency();
        freq -= 100000;
        wxGetApp().setFrequency(freq);
        ((wxFrame*) parent)->GetStatusBar()->SetStatusText(wxString::Format(wxT("Set center frequency: %i"), freq));
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

void WaterfallCanvas::setData(std::vector<signed char> *data) {

    if (data && data->size()) {
        if (spectrum_points.size() < FFT_SIZE * 2) {
            spectrum_points.resize(FFT_SIZE * 2);
        }

        for (int i = 0; i < BUF_SIZE / 2; i++) {
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

        for (int j = 0; j < 2; j++) {
            for (int i = 0, iMax = FFT_SIZE / 2; i < iMax; i++) {
                double a = out[i][0];
                double b = out[i][1];
                double c = sqrt(a * a + b * b);

                double x = out[FFT_SIZE / 2 + i][0];
                double y = out[FFT_SIZE / 2 + i][1];
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

        for (int i = 0, iMax = FFT_SIZE; i < iMax; i++) {
            float v = (log10(fft_result_maa[i] - fft_floor_maa) / log10(fft_ceil_maa - fft_floor_maa));
            spectrum_points[i * 2] = ((float) i / (float) iMax);
            spectrum_points[i * 2 + 1] = v;
        }

    }
}

void WaterfallCanvas::OnIdle(wxIdleEvent &event) {
//    timer.update();
//    frameTimer += timer.lastUpdateSeconds();
//    if (frameTimer > 1.0/30.0) {
    Refresh(false);
//        frameTimer = 0;
//    }
}

void WaterfallCanvas::mouseMoved(wxMouseEvent& event) {
    mTracker.OnMouseMoved(event);

    DemodulatorInstance *demod = wxGetApp().getDemodTest();

    if (mTracker.mouseDown()) {
        if (demod && mTracker.getDeltaMouseY()) {
            int bwDiff = (int)(mTracker.getDeltaMouseY() * 100000.0);

            if (!demodBW) {
                demodBW = demod->getParams().bandwidth;
            }

            DemodulatorThreadCommand command;
            command.cmd = DemodulatorThreadCommand::SDR_THREAD_CMD_SET_BANDWIDTH;
            demodBW = demodBW - bwDiff;
            if (demodBW < 1000) {
                demodBW = 1000;
            }
            if (demodBW > SRATE) {
                demodBW = SRATE;
            }

            command.int_value = demodBW;
            demod->getCommandQueue()->push(command);
            bwChange = true;
        }

//        int freqChange = mTracker.getDeltaMouseX() * SRATE;
//
//        if (freqChange != 0) {
//            int freq = wxGetApp().getFrequency();
//            freq -= freqChange;
//            wxGetApp().setFrequency(freq);
//
//            ((wxFrame*) parent)->GetStatusBar()->SetStatusText(
//                    wxString::Format(wxT("Set center frequency: %s"),
//                            wxNumberFormatter::ToString((long) freq, wxNumberFormatter::Style_WithThousandsSep)));
//        }
    }
}

void WaterfallCanvas::mouseDown(wxMouseEvent& event) {
    mTracker.OnMouseDown(event);
    SetCursor(wxCURSOR_SIZENS);
    bwChange = false;
}

void WaterfallCanvas::mouseWheelMoved(wxMouseEvent& event) {
    DemodulatorInstance *demod = wxGetApp().getDemodTest();
    mTracker.OnMouseWheelMoved(event);
}

void WaterfallCanvas::mouseReleased(wxMouseEvent& event) {
    mTracker.OnMouseReleased(event);

    if (mTracker.getOriginDeltaMouseX() == 0 && mTracker.getOriginDeltaMouseY() == 0 && !bwChange) {

        float pos = mTracker.getMouseX();

        int center_freq = wxGetApp().getFrequency();

        DemodulatorInstance *demod = wxGetApp().getDemodTest();

        int freq = center_freq - (int)(0.5 * (float)SRATE) + (int)((float)pos * (float)SRATE);

        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::SDR_THREAD_CMD_SET_FREQUENCY;
        command.int_value = freq;

        demod->getCommandQueue()->push(command);

        ((wxFrame*) parent)->GetStatusBar()->SetStatusText(
                wxString::Format(wxT("Set center frequency: %s"),
                        wxNumberFormatter::ToString((long) freq, wxNumberFormatter::Style_WithThousandsSep)));
    }

    SetCursor(wxCURSOR_CROSS);
}

void WaterfallCanvas::mouseLeftWindow(wxMouseEvent& event) {
    mTracker.OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
}

void WaterfallCanvas::mouseEnterWindow(wxMouseEvent& event) {
    mTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
}
