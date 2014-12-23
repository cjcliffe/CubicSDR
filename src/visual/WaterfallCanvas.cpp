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
EVT_KEY_UP(WaterfallCanvas::OnKeyUp)
EVT_IDLE(WaterfallCanvas::OnIdle)
EVT_MOTION(WaterfallCanvas::mouseMoved)
EVT_LEFT_DOWN(WaterfallCanvas::mouseDown)
EVT_LEFT_UP(WaterfallCanvas::mouseReleased)
EVT_LEAVE_WINDOW(WaterfallCanvas::mouseLeftWindow)
EVT_ENTER_WINDOW(WaterfallCanvas::mouseEnterWindow)
wxEND_EVENT_TABLE()

WaterfallCanvas::WaterfallCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
                   wxFULL_REPAINT_ON_RESIZE), parent(parent), frameTimer(0), activeDemodulatorBandwidth(0), activeDemodulatorFrequency(0), dragState(WF_DRAG_NONE), nextDragState(WF_DRAG_NONE), shiftDown(false), altDown(false), ctrlDown(false) {

    int in_block_size = FFT_SIZE;
    int out_block_size = FFT_SIZE;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * in_block_size);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * out_block_size);
    plan = fftw_plan_dft_1d(out_block_size, in, out, FFTW_FORWARD, FFTW_MEASURE);

    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;

    glContext = new WaterfallContext(this, &wxGetApp().GetContext(this));
    timer.start();

    mTracker.setTarget(this);
    SetCursor(wxCURSOR_CROSS);
}

WaterfallCanvas::~WaterfallCanvas() {

}

int WaterfallCanvas::GetFrequencyAt(float x) {

    int center_freq = wxGetApp().getFrequency();
    int freq = center_freq - (int) (0.5 * (float) SRATE) + (int) ((float) x * (float) SRATE);

    return freq;
}

void WaterfallCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->BeginDraw();
    glContext->Draw(spectrum_points);

    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();

    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    bool isNew = shiftDown || (wxGetApp().getDemodMgr().getLastActiveDemodulator() && !wxGetApp().getDemodMgr().getLastActiveDemodulator()->isActive());

    if (mTracker.mouseInView()) {
        if (nextDragState == WF_DRAG_RANGE) {
            if (mTracker.mouseDown()) {
                float width = mTracker.getOriginDeltaMouseX();
                float centerPos = mTracker.getOriginMouseX() + width / 2.0;

                if (isNew) {
                    glContext->DrawDemod(lastActiveDemodulator);
                    glContext->DrawFreqSelector(centerPos, 0, 1, 0, width ? width : (1.0 / (float) ClientSize.x));
                } else {
                    glContext->DrawDemod(lastActiveDemodulator, 1, 0, 0);
                    glContext->DrawFreqSelector(centerPos, 1, 1, 0, width ? width : (1.0 / (float) ClientSize.x));
                }
            } else {
                if (isNew) {
                    glContext->DrawDemod(lastActiveDemodulator);
                    glContext->DrawFreqSelector(mTracker.getMouseX(), 0, 1, 0, 1.0 / (float) ClientSize.x);
                } else {
                    glContext->DrawDemod(lastActiveDemodulator, 1, 0, 0);
                    glContext->DrawFreqSelector(mTracker.getMouseX(), 1, 1, 0, 1.0 / (float) ClientSize.x);
                }
            }
        } else {
            if (activeDemodulator == NULL) {
                if (lastActiveDemodulator) {
                    if (isNew) {
                        glContext->DrawDemod(lastActiveDemodulator);
                        glContext->DrawFreqSelector(mTracker.getMouseX(), 0, 1, 0);
                    } else {
                        glContext->DrawDemod(lastActiveDemodulator, 1, 0, 0);
                        glContext->DrawFreqSelector(mTracker.getMouseX(), 1, 1, 0);
                    }
                } else {
                    glContext->DrawFreqSelector(mTracker.getMouseX(), 1, 1, 0);
                }
            } else {
                if (lastActiveDemodulator) {
                    glContext->DrawDemod(lastActiveDemodulator);
                }
                glContext->DrawDemod(activeDemodulator, 1, 1, 0);
            }
        }
    } else {
        if (activeDemodulator) {
            glContext->DrawDemod(activeDemodulator);
        }
        if (lastActiveDemodulator) {
            glContext->DrawDemod(lastActiveDemodulator);
        }
    }

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        if (activeDemodulator == demods[i] || lastActiveDemodulator == demods[i]) {
            continue;
        }
        glContext->DrawDemod(demods[i]);
    }

    glContext->EndDraw();

    SwapBuffers();
}

void WaterfallCanvas::OnKeyUp(wxKeyEvent& event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
//    switch (event.GetKeyCode()) {
//    }
}

void WaterfallCanvas::OnKeyDown(wxKeyEvent& event) {
    float angle = 5.0;

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    DemodulatorInstance *activeDemod = wxGetApp().getDemodMgr().getActiveDemodulator();

    unsigned int freq;
    switch (event.GetKeyCode()) {
    case WXK_RIGHT:
        freq = wxGetApp().getFrequency();
        if (shiftDown) {
            freq += SRATE*10;
        } else {
            freq += SRATE / 2;
        }
        wxGetApp().setFrequency(freq);
        ((wxFrame*) parent)->GetStatusBar()->SetStatusText(wxString::Format(wxT("Set center frequency: %i"), freq));
        break;
    case WXK_LEFT:
        freq = wxGetApp().getFrequency();
        if (shiftDown) {
            freq -= SRATE*10;
        } else {
            freq -= SRATE / 2;
        }
        wxGetApp().setFrequency(freq);
        ((wxFrame*) parent)->GetStatusBar()->SetStatusText(wxString::Format(wxT("Set center frequency: %i"), freq));
        break;
    case 'D':
    case WXK_DELETE:
        if (!activeDemod) {
            break;
        }
        wxGetApp().removeDemodulator(activeDemod);
        wxGetApp().getDemodMgr().deleteThread(activeDemod);
        break;
    case 'S':
        if (!activeDemod) {
            break;
        }
        if (activeDemod->isSquelchEnabled()) {
            activeDemod->setSquelchEnabled(false);
        } else {
            activeDemod->squelchAuto();
        }
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

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getActiveDemodulator();

    if (mTracker.mouseDown()) {
        if (demod == NULL) {
            return;
        }
        if (dragState == WF_DRAG_BANDWIDTH_LEFT || dragState == WF_DRAG_BANDWIDTH_RIGHT) {

            int bwDiff = (int) (mTracker.getDeltaMouseX() * (float) SRATE) * 2;

            if (dragState == WF_DRAG_BANDWIDTH_LEFT) {
                bwDiff = -bwDiff;
            }

            if (!activeDemodulatorBandwidth) {
                activeDemodulatorBandwidth = demod->getParams().bandwidth;
            }

            DemodulatorThreadCommand command;
            command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH;
            activeDemodulatorBandwidth = activeDemodulatorBandwidth + bwDiff;
            if (activeDemodulatorBandwidth < 1000) {
                activeDemodulatorBandwidth = 1000;
            }
            if (activeDemodulatorBandwidth > SRATE) {
                activeDemodulatorBandwidth = SRATE;
            }

            command.int_value = activeDemodulatorBandwidth;
            demod->getCommandQueue()->push(command);
        }

        if (dragState == WF_DRAG_FREQUENCY) {
            int bwDiff = (int) (mTracker.getDeltaMouseX() * (float) SRATE);

            if (!activeDemodulatorFrequency) {
                activeDemodulatorFrequency = demod->getParams().frequency;
            }

            DemodulatorThreadCommand command;
            command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY;
            activeDemodulatorFrequency = activeDemodulatorFrequency + bwDiff;

            command.int_value = activeDemodulatorFrequency;
            demod->getCommandQueue()->push(command);

            demod->updateLabel(activeDemodulatorFrequency);
        }
    } else {
        int freqPos = GetFrequencyAt(mTracker.getMouseX());

        std::vector<DemodulatorInstance *> *demodsHover = wxGetApp().getDemodMgr().getDemodulatorsAt(freqPos, 15000);

        wxGetApp().getDemodMgr().setActiveDemodulator(NULL);

        if (altDown) {
            nextDragState = WF_DRAG_RANGE;
            mTracker.setVertDragLock(true);
            mTracker.setHorizDragLock(false);
        } else if (demodsHover->size()) {
            int hovered = -1;
            int near_dist = SRATE;

            DemodulatorInstance *activeDemodulator = NULL;

            for (int i = 0, iMax = demodsHover->size(); i < iMax; i++) {
                DemodulatorInstance *demod = (*demodsHover)[i];
                int freqDiff = (int) demod->getParams().frequency - freqPos;
                int halfBw = (demod->getParams().bandwidth / 2);

                int dist = abs(freqDiff);

                if (dist < near_dist) {
                    activeDemodulator = demod;
                    near_dist = dist;
                }

                if (dist <= halfBw && dist >= (int) ((float) halfBw / (float) 1.5)) {
                    int edge_dist = abs(halfBw - dist);
                    if (edge_dist < near_dist) {
                        activeDemodulator = demod;
                        near_dist = edge_dist;
                    }
                }
            }

            if (activeDemodulator == NULL) {
                return;
            }

            wxGetApp().getDemodMgr().setActiveDemodulator(activeDemodulator);

            int freqDiff = ((int) activeDemodulator->getParams().frequency - freqPos);

            if (abs(freqDiff) > (activeDemodulator->getParams().bandwidth / 3)) {
                SetCursor(wxCURSOR_SIZEWE);

                if (freqDiff > 0) {
                    nextDragState = WF_DRAG_BANDWIDTH_LEFT;
                } else {
                    nextDragState = WF_DRAG_BANDWIDTH_RIGHT;
                }

                mTracker.setVertDragLock(true);
                mTracker.setHorizDragLock(false);
            } else {
                SetCursor(wxCURSOR_SIZING);
                nextDragState = WF_DRAG_FREQUENCY;

                mTracker.setVertDragLock(true);
                mTracker.setHorizDragLock(false);
            }
        } else {
            SetCursor(wxCURSOR_CROSS);
            nextDragState = WF_DRAG_NONE;
        }

        delete demodsHover;
    }
}

void WaterfallCanvas::mouseDown(wxMouseEvent& event) {
    mTracker.OnMouseDown(event);
    dragState = nextDragState;

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    if (dragState && dragState != WF_DRAG_RANGE) {
        wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getActiveDemodulator(), false);
    }

    activeDemodulatorBandwidth = 0;
    activeDemodulatorFrequency = 0;
}

void WaterfallCanvas::mouseWheelMoved(wxMouseEvent& event) {
    mTracker.OnMouseWheelMoved(event);
}

void WaterfallCanvas::mouseReleased(wxMouseEvent& event) {
    mTracker.OnMouseReleased(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    bool isNew = shiftDown || (wxGetApp().getDemodMgr().getLastActiveDemodulator() && !wxGetApp().getDemodMgr().getLastActiveDemodulator()->isActive());

    mTracker.setVertDragLock(false);
    mTracker.setHorizDragLock(false);

    DemodulatorInstance *demod;

    if (mTracker.getOriginDeltaMouseX() == 0 && mTracker.getOriginDeltaMouseY() == 0) {
        float pos = mTracker.getMouseX();
        int center_freq = wxGetApp().getFrequency();
        int freq = center_freq - (int) (0.5 * (float) SRATE) + (int) ((float) pos * (float) SRATE);

        if (dragState == WF_DRAG_NONE) {
            if (!isNew && wxGetApp().getDemodMgr().getDemodulators().size()) {
                demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
            } else {
                demod = wxGetApp().getDemodMgr().newThread();
                demod->getParams().frequency = freq;

                if (DemodulatorInstance *last = wxGetApp().getDemodMgr().getLastActiveDemodulator()) {
                    demod->getParams().bandwidth = last->getParams().bandwidth;
                }

                demod->run();

                wxGetApp().bindDemodulator(demod);
                wxGetApp().getDemodMgr().setActiveDemodulator(demod, false);
            }

            if (demod == NULL) {
                dragState = WF_DRAG_NONE;
                return;
            }

            demod->updateLabel(freq);

            DemodulatorThreadCommand command;
            command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY;
            command.int_value = freq;
            demod->getCommandQueue()->push(command);

            ((wxFrame*) parent)->GetStatusBar()->SetStatusText(
                    wxString::Format(wxT("Set demodulator frequency: %s"),
                            wxNumberFormatter::ToString((long) freq, wxNumberFormatter::Style_WithThousandsSep)));

            wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getLastActiveDemodulator(), false);
            SetCursor(wxCURSOR_SIZING);
            nextDragState = WF_DRAG_FREQUENCY;
            mTracker.setVertDragLock(true);
            mTracker.setHorizDragLock(false);
        } else {
            float pos = mTracker.getMouseX();
            int center_freq = wxGetApp().getFrequency();
            int freq = center_freq - (int) (0.5 * (float) SRATE) + (int) ((float) pos * (float) SRATE);

            wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getActiveDemodulator(), false);
            nextDragState = WF_DRAG_FREQUENCY;
        }
    } else if (dragState == WF_DRAG_RANGE) {
        float width = mTracker.getOriginDeltaMouseX();
        float pos = mTracker.getOriginMouseX() + width / 2.0;

        int center_freq = wxGetApp().getFrequency();
        int freq = center_freq - (int) (0.5 * (float) SRATE) + (int) ((float) pos * (float) SRATE);
        int bandwidth = (int) (fabs(width) * (float) SRATE);

        if (bandwidth < 1000) {
            bandwidth = 1000;
        }

        if (!bandwidth) {
            dragState = WF_DRAG_NONE;
            return;
        }

        if (!isNew && wxGetApp().getDemodMgr().getDemodulators().size()) {
            demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
        } else {
            demod = wxGetApp().getDemodMgr().newThread();
            demod->getParams().frequency = freq;
            demod->getParams().bandwidth = bandwidth;

            demod->run();

            wxGetApp().bindDemodulator(demod);
            wxGetApp().getDemodMgr().setActiveDemodulator(demod, false);
        }

        if (demod == NULL) {
            dragState = WF_DRAG_NONE;
            return;
        }

        ((wxFrame*) parent)->GetStatusBar()->SetStatusText(
                wxString::Format(wxT("Set demodulator frequency: %s"),
                        wxNumberFormatter::ToString((long) freq, wxNumberFormatter::Style_WithThousandsSep)));

        wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getLastActiveDemodulator(), false);
        demod->updateLabel(freq);

        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY;
        command.int_value = freq;
        demod->getCommandQueue()->push(command);
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH;
        command.int_value = bandwidth;
        demod->getCommandQueue()->push(command);
    }

    dragState = WF_DRAG_NONE;
}

void WaterfallCanvas::mouseLeftWindow(wxMouseEvent& event) {
    mTracker.OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
    wxGetApp().getDemodMgr().setActiveDemodulator(NULL);
}

void WaterfallCanvas::mouseEnterWindow(wxMouseEvent& event) {
    mTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
}
