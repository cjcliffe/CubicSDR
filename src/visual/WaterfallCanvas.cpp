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

#define MIN_BANDWIDTH 1500

wxBEGIN_EVENT_TABLE(WaterfallCanvas, wxGLCanvas) EVT_PAINT(WaterfallCanvas::OnPaint)
EVT_KEY_DOWN(WaterfallCanvas::OnKeyDown)
EVT_KEY_UP(WaterfallCanvas::OnKeyUp)
EVT_IDLE(WaterfallCanvas::OnIdle)
EVT_MOTION(WaterfallCanvas::OnMouseMoved)
EVT_LEFT_DOWN(WaterfallCanvas::OnMouseDown)
EVT_LEFT_UP(WaterfallCanvas::OnMouseReleased)
EVT_RIGHT_DOWN(WaterfallCanvas::OnMouseRightDown)
EVT_RIGHT_UP(WaterfallCanvas::OnMouseRightReleased)
EVT_LEAVE_WINDOW(WaterfallCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(WaterfallCanvas::OnMouseEnterWindow)
EVT_MOUSEWHEEL(WaterfallCanvas::OnMouseWheelMoved)
wxEND_EVENT_TABLE()

WaterfallCanvas::WaterfallCanvas(wxWindow *parent, int *attribList) :
        InteractiveCanvas(parent, attribList), spectrumCanvas(NULL), dragState(WF_DRAG_NONE), nextDragState(WF_DRAG_NONE), fft_size(0), waterfall_lines(
                0), plan(
        NULL), in(NULL), out(NULL), resampler(NULL), resamplerRatio(0), lastInputBandwidth(0), zoom(1), mouseZoom(1), otherWaterfallCanvas(NULL), polling(true), last_data_size(0), fft_in_data(NULL), fft_last_data(NULL), hoverAlpha(1.0) {

    glContext = new WaterfallContext(this, &wxGetApp().GetContext(this));

    freqShifter = nco_crcf_create(LIQUID_NCO);
    shiftFrequency = 0;

    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;

    SetCursor(wxCURSOR_CROSS);
}

WaterfallCanvas::~WaterfallCanvas() {
    nco_crcf_destroy(freqShifter);
}

void WaterfallCanvas::setup(int fft_size_in, int waterfall_lines_in) {
    if (fft_size == fft_size_in && waterfall_lines_in == waterfall_lines) {
        return;
    }
    fft_size = fft_size_in;
    waterfall_lines = waterfall_lines_in;

    if (in) {
        free(in);
    }
    in = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    if (fft_in_data) {
        free(fft_in_data);
    }
    fft_in_data = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    if (fft_last_data) {
        free(fft_last_data);
    }
    fft_last_data = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    if (out) {
        free(out);
    }
    out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    if (plan) {
        fftwf_destroy_plan(plan);
    }
    plan = fftwf_plan_dft_1d(fft_size, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    glContext->Setup(fft_size, waterfall_lines);
}

WaterfallCanvas::DragState WaterfallCanvas::getDragState() {
    return dragState;
}

WaterfallCanvas::DragState WaterfallCanvas::getNextDragState() {
    return nextDragState;
}

void WaterfallCanvas::attachSpectrumCanvas(SpectrumCanvas *canvas_in) {
    spectrumCanvas = canvas_in;
}

void WaterfallCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
#ifdef __APPLE__    // force half-rate?
    glFinish();
#endif

    const wxSize ClientSize = GetClientSize();


    if (polling && !wxGetApp().getIQVisualQueue()->empty()) {
        DemodulatorThreadIQData *iqData;
        wxGetApp().getIQVisualQueue()->pop(iqData);

        if (iqData && iqData->data.size()) {
            setData(iqData);
            if (otherWaterfallCanvas) {
                otherWaterfallCanvas->setData(iqData);
            }
        } else {
            std::cout << "Incoming IQ data empty?" << std::endl;
        }
    }

    glContext->SetCurrent(*this);
    initGLExtensions();
   glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->BeginDraw(0,0,0);
    glContext->Draw(spectrum_points);

    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();

    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    bool isNew = shiftDown
            || (wxGetApp().getDemodMgr().getLastActiveDemodulator() && !wxGetApp().getDemodMgr().getLastActiveDemodulator()->isActive());

    int currentBandwidth = getBandwidth();
    long long currentCenterFreq = getCenterFrequency();

    float demodColor, selectorColor;
    ColorTheme *currentTheme = ThemeMgr::mgr.currentTheme;
    int last_type = wxGetApp().getDemodMgr().getLastDemodulatorType();

    if (mouseTracker.mouseInView()) {
        hoverAlpha += (1.0f-hoverAlpha)*0.1f;
        if (hoverAlpha > 1.5f) {
            hoverAlpha = 1.5f;
        }
        glContext->setHoverAlpha(hoverAlpha);
        if (nextDragState == WF_DRAG_RANGE) {
            float width = (1.0 / (float) ClientSize.x);
            float rangeWidth = mouseTracker.getOriginDeltaMouseX();
            float centerPos;

            if (mouseTracker.mouseDown()) {
                if (rangeWidth) {
                    width = rangeWidth;
                }
                centerPos = mouseTracker.getOriginMouseX() + width / 2.0;
            } else {
                centerPos = mouseTracker.getMouseX();
            }

            glContext->DrawDemod(lastActiveDemodulator, isNew?currentTheme->waterfallHighlight:currentTheme->waterfallDestroy, currentCenterFreq, currentBandwidth);

            if ((last_type == DEMOD_TYPE_LSB || last_type == DEMOD_TYPE_USB) && mouseTracker.mouseDown()) {
                centerPos = mouseTracker.getMouseX();
                glContext->DrawRangeSelector(centerPos, centerPos-width, isNew?currentTheme->waterfallNew:currentTheme->waterfallHover);
            } else {
                glContext->DrawFreqSelector(centerPos, isNew?currentTheme->waterfallNew:currentTheme->waterfallHover, width, currentCenterFreq, currentBandwidth);
            }
        } else {
            if (lastActiveDemodulator) {
                glContext->DrawDemod(lastActiveDemodulator, ((isNew && activeDemodulator == NULL) || (activeDemodulator != NULL))?currentTheme->waterfallHighlight:currentTheme->waterfallDestroy, currentCenterFreq, currentBandwidth);
            }
            if (activeDemodulator == NULL) {
                glContext->DrawFreqSelector(mouseTracker.getMouseX(), ((isNew && lastActiveDemodulator) || (!lastActiveDemodulator) )?currentTheme->waterfallNew:currentTheme->waterfallHover, 0, currentCenterFreq, currentBandwidth);
            } else {
                glContext->DrawDemod(activeDemodulator, currentTheme->waterfallHover, currentCenterFreq, currentBandwidth);
            }
        }
    } else {
        hoverAlpha += (0.0f-hoverAlpha)*0.05f;
        if (hoverAlpha < 1.0e-5f) {
            hoverAlpha = 0;
        }
        glContext->setHoverAlpha(hoverAlpha);
        if (activeDemodulator) {
            glContext->DrawDemod(activeDemodulator, currentTheme->waterfallHighlight, currentCenterFreq, currentBandwidth);
        }
        if (lastActiveDemodulator) {
            glContext->DrawDemod(lastActiveDemodulator, currentTheme->waterfallHighlight, currentCenterFreq, currentBandwidth);
        }
    }
    std::cout<<hoverAlpha<<std::endl;

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        if (activeDemodulator == demods[i] || lastActiveDemodulator == demods[i]) {
            continue;
        }
        glContext->DrawDemod(demods[i], currentTheme->waterfallHighlight, currentCenterFreq, currentBandwidth);
    }

    glContext->EndDraw();

    SwapBuffers();
}

void WaterfallCanvas::OnKeyUp(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyUp(event);
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
    switch (event.GetKeyCode()) {
    case 'A':
        zoom = 1.0;
        break;
    case 'Z':
        zoom = 1.0;
        break;
    }
}

void WaterfallCanvas::OnKeyDown(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyDown(event);
    float angle = 5.0;

    DemodulatorInstance *activeDemod = wxGetApp().getDemodMgr().getActiveDemodulator();

    long long freq;
    long long originalFreq;
    unsigned int bw;
    switch (event.GetKeyCode()) {
    case 'A':
        zoom = 0.95;
        break;
    case 'Z':
        zoom = 1.05;
        break;
    case WXK_RIGHT:
        freq = wxGetApp().getFrequency();
        originalFreq = freq;
        if (shiftDown) {
            freq += wxGetApp().getSampleRate() * 10;
            if (isView) {
                setView(centerFreq + (freq - originalFreq), getBandwidth());
                if (spectrumCanvas) {
                    spectrumCanvas->setView(getCenterFrequency(), getBandwidth());
                }
            }
        } else {
            freq += wxGetApp().getSampleRate() / 2;
            if (isView) {
                setView(centerFreq + (freq - originalFreq), getBandwidth());
                if (spectrumCanvas) {
                    spectrumCanvas->setView(getCenterFrequency(), getBandwidth());
                }
            }
        }
        wxGetApp().setFrequency(freq);
        setStatusText("Set center frequency: %s", freq);
        break;
    case WXK_LEFT:
        freq = wxGetApp().getFrequency();
        originalFreq = freq;
        if (shiftDown) {
            if ((freq - wxGetApp().getSampleRate() * 10) < wxGetApp().getSampleRate() / 2) {
                freq = wxGetApp().getSampleRate() / 2;
            } else {
                freq -= wxGetApp().getSampleRate() * 10;
            }
            if (isView) {
                setView(centerFreq + (freq - originalFreq), getBandwidth());
                if (spectrumCanvas) {
                    spectrumCanvas->setView(getCenterFrequency(), getBandwidth());
                }
            }
        } else {
            if ((freq - wxGetApp().getSampleRate() / 2) < wxGetApp().getSampleRate() / 2) {
                freq = wxGetApp().getSampleRate() / 2;
            } else {
                freq -= wxGetApp().getSampleRate() / 2;
            }
            if (isView) {
                setView(centerFreq + (freq - originalFreq), getBandwidth());
                if (spectrumCanvas) {
                    spectrumCanvas->setView(getCenterFrequency(), getBandwidth());
                }
            }
        }
        wxGetApp().setFrequency(freq);
        setStatusText("Set center frequency: %s", freq);
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
    case WXK_SPACE:
        if (!activeDemod) {
            break;
        }
        if (activeDemod->isStereo()) {
            activeDemod->setStereo(false);
        } else {
            activeDemod->setStereo(true);
        }
        break;
    default:
        event.Skip();
        return;
    }
}

void WaterfallCanvas::setData(DemodulatorThreadIQData *input) {
    if (!input) {
        return;
    }

    long double currentZoom = zoom;

    if (mouseZoom != 1) {
        currentZoom = mouseZoom;
        mouseZoom = mouseZoom + (1.0 - mouseZoom) * 0.2;
        if (fabs(mouseZoom-1.0)<0.01) {
            mouseZoom = 1;
        }
    }

    long long bw;
    if (currentZoom != 1) {
        long long freq = wxGetApp().getFrequency();

        if (currentZoom < 1) {
            centerFreq = getCenterFrequency();
            bw = getBandwidth();
            bw = (long long) ceil((long double) bw * currentZoom);
            if (bw < 100000) {
                bw = 100000;
            }
            if (mouseTracker.mouseInView()) {
                long long mfreqA = getFrequencyAt(mouseTracker.getMouseX());
                setBandwidth(bw);
                long long mfreqB = getFrequencyAt(mouseTracker.getMouseX());
                centerFreq += mfreqA - mfreqB;
            }

            setView(centerFreq, bw);
            if (spectrumCanvas) {
                spectrumCanvas->setView(centerFreq, bw);
            }
        } else {
            if (isView) {
                bw = getBandwidth();
                bw = (long long) ceil((long double) bw * currentZoom);
                if (bw >= wxGetApp().getSampleRate()) {
                    bw = wxGetApp().getSampleRate();
                    disableView();
                    if (spectrumCanvas) {
                        spectrumCanvas->disableView();
                    }
                } else {
                    if (mouseTracker.mouseInView()) {
                        long long freq = wxGetApp().getFrequency();
                        long long mfreqA = getFrequencyAt(mouseTracker.getMouseX());
                        setBandwidth(bw);
                        long long mfreqB = getFrequencyAt(mouseTracker.getMouseX());
                        centerFreq += mfreqA - mfreqB;
                    }

                    setView(getCenterFrequency(), bw);
                    if (spectrumCanvas) {
                        spectrumCanvas->setView(centerFreq, bw);
                    }
                }
            }
        }
        if (centerFreq < freq && (centerFreq - bandwidth / 2) < (freq - wxGetApp().getSampleRate() / 2)) {
            centerFreq = (freq - wxGetApp().getSampleRate() / 2) + bandwidth / 2;
        }
        if (centerFreq > freq && (centerFreq + bandwidth / 2) > (freq + wxGetApp().getSampleRate() / 2)) {
            centerFreq = (freq + wxGetApp().getSampleRate() / 2) - bandwidth / 2;
        }
    }

    std::vector<liquid_float_complex> *data = &input->data;

    if (data && data->size()) {
//        if (fft_size != data->size() && !isView) {
//            Setup(data->size(), waterfall_lines);
//        }

//        if (last_bandwidth != bandwidth && !isView) {
//            Setup(bandwidth, waterfall_lines);
//        }

        if (spectrum_points.size() < fft_size * 2) {
            spectrum_points.resize(fft_size * 2);
        }

        unsigned int num_written;

        if (isView) {
            if (!input->frequency || !input->sampleRate) {
                return;
            }

            resamplerRatio = (double) (bandwidth) / (double) input->sampleRate;

            int desired_input_size = fft_size / resamplerRatio;

            if (input->data.size() < desired_input_size) {
//                std::cout << "fft underflow, desired: " << desired_input_size << " actual:" << input->data.size() << std::endl;
                desired_input_size = input->data.size();
            }

            if (centerFreq != input->frequency) {
                if ((centerFreq - input->frequency) != shiftFrequency || lastInputBandwidth != input->sampleRate) {
                    if (abs(input->frequency - centerFreq) < (wxGetApp().getSampleRate() / 2)) {
                        shiftFrequency = centerFreq - input->frequency;
                        nco_crcf_reset(freqShifter);
                        nco_crcf_set_frequency(freqShifter, (2.0 * M_PI) * (((double) abs(shiftFrequency)) / ((double) input->sampleRate)));
                    }
                }

                if (shiftBuffer.size() != desired_input_size) {
                    if (shiftBuffer.capacity() < desired_input_size) {
                        shiftBuffer.reserve(desired_input_size);
                    }
                    shiftBuffer.resize(desired_input_size);
                }

                if (shiftFrequency < 0) {
                    nco_crcf_mix_block_up(freqShifter, &input->data[0], &shiftBuffer[0], desired_input_size);
                } else {
                    nco_crcf_mix_block_down(freqShifter, &input->data[0], &shiftBuffer[0], desired_input_size);
                }
            } else {
                shiftBuffer.assign(input->data.begin(), input->data.end());
            }

            if (!resampler || bandwidth != lastBandwidth || lastInputBandwidth != input->sampleRate) {
                float As = 60.0f;

                if (resampler) {
                    msresamp_crcf_destroy(resampler);
                }
                resampler = msresamp_crcf_create(resamplerRatio, As);

                lastBandwidth = bandwidth;
                lastInputBandwidth = input->sampleRate;
            }


            int out_size = ceil((double) (desired_input_size) * resamplerRatio) + 512;

            if (resampleBuffer.size() != out_size) {
                if (resampleBuffer.capacity() < out_size) {
                    resampleBuffer.reserve(out_size);
                }
                resampleBuffer.resize(out_size);
            }


            msresamp_crcf_execute(resampler, &shiftBuffer[0], desired_input_size, &resampleBuffer[0], &num_written);

            resampleBuffer.resize(fft_size);

            if (num_written < fft_size) {
                for (int i = 0; i < num_written; i++) {
                    fft_in_data[i][0] = resampleBuffer[i].real;
                    fft_in_data[i][1] = resampleBuffer[i].imag;
                }
                for (int i = num_written; i < fft_size; i++) {
                    fft_in_data[i][0] = 0;
                    fft_in_data[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fft_size; i++) {
                    fft_in_data[i][0] = resampleBuffer[i].real;
                    fft_in_data[i][1] = resampleBuffer[i].imag;
                }
            }
        } else {
            num_written = data->size();
            if (data->size() < fft_size) {
                for (int i = 0, iMax = data->size(); i < iMax; i++) {
                    fft_in_data[i][0] = (*data)[i].real;
                    fft_in_data[i][1] = (*data)[i].imag;
                }
                for (int i = data->size(); i < fft_size; i++) {
                    fft_in_data[i][0] = 0;
                    fft_in_data[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fft_size; i++) {
                    fft_in_data[i][0] = (*data)[i].real;
                    fft_in_data[i][1] = (*data)[i].imag;
                }
            }
        }

        bool execute = false;

        if (num_written >= fft_size) {
            execute = true;
            memcpy(in, fft_in_data, fft_size * sizeof(fftwf_complex));
            memcpy(fft_last_data, in, fft_size * sizeof(fftwf_complex));

        } else {
            if (last_data_size + num_written < fft_size) { // priming
                unsigned int num_copy = fft_size;
                num_copy = fft_size - last_data_size;
                if (num_written > num_copy) {
                    num_copy = num_written;
                }
                memcpy(fft_last_data, fft_in_data, num_copy * sizeof(fftwf_complex));
                last_data_size += num_copy;
            } else {
                unsigned int num_last = (fft_size - num_written);
                memcpy(in, fft_last_data + (last_data_size - num_last), num_last * sizeof(fftwf_complex));
                memcpy(in + num_last, fft_in_data, num_written * sizeof(fftwf_complex));
                memcpy(fft_last_data, in, fft_size * sizeof(fftwf_complex));
                execute = true;
            }
        }

        if (execute) {
            fftwf_execute(plan);

            float fft_ceil = 0, fft_floor = 1;

            if (fft_result.size() < fft_size) {
                fft_result.resize(fft_size);
                fft_result_ma.resize(fft_size);
                fft_result_maa.resize(fft_size);
            }

            int n;
            for (int i = 0, iMax = fft_size / 2; i < iMax; i++) {
                float a = out[i][0];
                float b = out[i][1];
                float c = sqrt(a * a + b * b);

                float x = out[fft_size / 2 + i][0];
                float y = out[fft_size / 2 + i][1];
                float z = sqrt(x * x + y * y);

                fft_result[i] = (z);
                fft_result[fft_size / 2 + i] = (c);
            }

            for (int i = 0, iMax = fft_size; i < iMax; i++) {
                if (isView) {
                    fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * 0.65;
                    fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * 0.65;
                } else {
                    fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * 0.65;
                    fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * 0.65;
                }

                if (fft_result_maa[i] > fft_ceil) {
                    fft_ceil = fft_result_maa[i];
                }
                if (fft_result_maa[i] < fft_floor) {
                    fft_floor = fft_result_maa[i];
                }
            }

            fft_ceil += 0.25;
            fft_floor -= 1;

            fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.05;
            fft_ceil_maa = fft_ceil_maa + (fft_ceil_ma - fft_ceil_maa) * 0.05;

            fft_floor_ma = fft_floor_ma + (fft_floor - fft_floor_ma) * 0.05;
            fft_floor_maa = fft_floor_maa + (fft_floor_ma - fft_floor_maa) * 0.05;

            for (int i = 0, iMax = fft_size; i < iMax; i++) {
                float v = (log10(fft_result_maa[i] - fft_floor_maa) / log10(fft_ceil_maa - fft_floor_maa));
                spectrum_points[i * 2] = ((float) i / (float) iMax);
                spectrum_points[i * 2 + 1] = v;
            }

            if (spectrumCanvas) {
                spectrumCanvas->spectrum_points.assign(spectrum_points.begin(), spectrum_points.end());
                spectrumCanvas->getSpectrumContext()->setCeilValue(fft_ceil_maa);
                spectrumCanvas->getSpectrumContext()->setFloorValue(fft_floor_maa);
            }
        }
    }
}

void WaterfallCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

void WaterfallCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);
    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getActiveDemodulator();

    if (mouseTracker.mouseDown()) {
        if (demod == NULL) {
            return;
        }
        if (dragState == WF_DRAG_BANDWIDTH_LEFT || dragState == WF_DRAG_BANDWIDTH_RIGHT) {

            int bwDiff = (int) (mouseTracker.getDeltaMouseX() * (float) getBandwidth()) * 2;

            if (dragState == WF_DRAG_BANDWIDTH_LEFT) {
                bwDiff = -bwDiff;
            }

            int currentBW = demod->getBandwidth();

            currentBW = currentBW + bwDiff;
            if (currentBW > wxGetApp().getSampleRate()) {
                currentBW = wxGetApp().getSampleRate();
            }
            if (currentBW < MIN_BANDWIDTH) {
                currentBW = MIN_BANDWIDTH;
            }

            demod->setBandwidth(currentBW);
            setStatusText("Set demodulator bandwidth: %s", demod->getBandwidth());
        }

        if (dragState == WF_DRAG_FREQUENCY) {
            long long bwDiff = (long long) (mouseTracker.getDeltaMouseX() * (float) getBandwidth());
            long long currentFreq = demod->getFrequency();

            demod->setFrequency(currentFreq + bwDiff);
            currentFreq = demod->getFrequency();
            demod->updateLabel(currentFreq);

            setStatusText("Set demodulator frequency: %s", demod->getFrequency());
        }
    } else if (mouseTracker.mouseRightDown()) {
        mouseZoom = mouseZoom + ((1.0 - (mouseTracker.getDeltaMouseY() * 4.0)) - mouseZoom) * 0.1;
    } else {
        long long freqPos = getFrequencyAt(mouseTracker.getMouseX());

        std::vector<DemodulatorInstance *> *demodsHover = wxGetApp().getDemodMgr().getDemodulatorsAt(freqPos, 15000);

        wxGetApp().getDemodMgr().setActiveDemodulator(NULL);

        if (altDown) {
            nextDragState = WF_DRAG_RANGE;
            mouseTracker.setVertDragLock(true);
            mouseTracker.setHorizDragLock(false);
            if (shiftDown) {
                setStatusText("Click and drag to create a new demodulator by range.");
            } else {
                setStatusText("Click and drag to set the current demodulator range.");
            }
        } else if (demodsHover->size() && !shiftDown) {
            int hovered = -1;
            long near_dist = getBandwidth();

            DemodulatorInstance *activeDemodulator = NULL;

            for (int i = 0, iMax = demodsHover->size(); i < iMax; i++) {
                DemodulatorInstance *demod = (*demodsHover)[i];
                long long freqDiff = demod->getFrequency() - freqPos;
                long halfBw = (demod->getBandwidth() / 2);

                long dist = abs(freqDiff);

                if (dist < near_dist) {
                    activeDemodulator = demod;
                    near_dist = dist;
                }

                if (dist <= halfBw && dist >= (int) ((float) halfBw / (1.5 - (0.65 * (1.0-(float)(wxGetApp().getSampleRate() - getBandwidth())/(float)wxGetApp().getSampleRate()))))) {
                    long edge_dist = abs(halfBw - dist);
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

            long long freqDiff = activeDemodulator->getFrequency() - freqPos;

            if (abs(freqDiff) > (activeDemodulator->getBandwidth() / 3)) {
                SetCursor(wxCURSOR_SIZEWE);

                if (freqDiff > 0) {
                    nextDragState = WF_DRAG_BANDWIDTH_LEFT;
                } else {
                    nextDragState = WF_DRAG_BANDWIDTH_RIGHT;
                }

                mouseTracker.setVertDragLock(true);
                mouseTracker.setHorizDragLock(false);
                setStatusText("Click and drag to change demodulator bandwidth. D to delete, SPACE for stereo.");
            } else {
                SetCursor(wxCURSOR_SIZING);
                nextDragState = WF_DRAG_FREQUENCY;

                mouseTracker.setVertDragLock(true);
                mouseTracker.setHorizDragLock(false);
                setStatusText("Click and drag to change demodulator frequency. D to delete, SPACE for stereo.");
            }
        } else {
            SetCursor(wxCURSOR_CROSS);
            nextDragState = WF_DRAG_NONE;
            if (shiftDown) {
                setStatusText("Click to create a new demodulator or hold ALT to drag range.");
            } else {
                setStatusText(
                        "Click to move active demodulator frequency or hold ALT to drag range; hold SHIFT to create new.  Right drag or A / Z to Zoom.  Arrow keys (+SHIFT) to move center frequency.");
            }
        }

        delete demodsHover;
    }
}

void WaterfallCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);

    dragState = nextDragState;

    if (dragState && dragState != WF_DRAG_RANGE) {
        wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getActiveDemodulator(), false);
    }
}

void WaterfallCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
    float movement = (float)event.GetWheelRotation() / (float)event.GetLinesPerAction();

    mouseZoom = 1.0f - movement/1000.0f;
}

void WaterfallCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);

    bool isNew = shiftDown || (wxGetApp().getDemodMgr().getLastActiveDemodulator() == NULL)
            || (wxGetApp().getDemodMgr().getLastActiveDemodulator() && !wxGetApp().getDemodMgr().getLastActiveDemodulator()->isActive());

    mouseTracker.setVertDragLock(false);
    mouseTracker.setHorizDragLock(false);

    DemodulatorInstance *demod;
    DemodulatorMgr *mgr = &wxGetApp().getDemodMgr();

    if (mouseTracker.getOriginDeltaMouseX() == 0 && mouseTracker.getOriginDeltaMouseY() == 0) {
        float pos = mouseTracker.getMouseX();
        long long input_center_freq = getCenterFrequency();
        long long freq = input_center_freq - (long long) (0.5 * (float) getBandwidth()) + (long long) ((float) pos * (float) getBandwidth());

        if (dragState == WF_DRAG_NONE) {
            if (!isNew && wxGetApp().getDemodMgr().getDemodulators().size()) {
                demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
            } else {
                isNew = true;
                demod = wxGetApp().getDemodMgr().newThread();
                demod->setFrequency(freq);

                demod->setBandwidth(mgr->getLastBandwidth());
                demod->setDemodulatorType(mgr->getLastDemodulatorType());
                demod->setSquelchLevel(mgr->getLastSquelchLevel());
                demod->setSquelchEnabled(mgr->isLastSquelchEnabled());
                demod->setStereo(mgr->isLastStereo());
                demod->setGain(mgr->getLastGain());

                demod->run();

                wxGetApp().bindDemodulator(demod);
                wxGetApp().getDemodMgr().setActiveDemodulator(demod, false);
            }

            if (demod == NULL) {
                dragState = WF_DRAG_NONE;
                return;
            }

            demod->updateLabel(freq);
            demod->setFrequency(freq);

            if (isNew) {
                setStatusText("New demodulator at frequency: %s", freq);
            } else {
                setStatusText("Moved demodulator to frequency: %s", freq);
            }

            wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getLastActiveDemodulator(), false);
            SetCursor(wxCURSOR_SIZING);
            nextDragState = WF_DRAG_FREQUENCY;
            mouseTracker.setVertDragLock(true);
            mouseTracker.setHorizDragLock(false);
        } else {
            wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getActiveDemodulator(), false);
            nextDragState = WF_DRAG_FREQUENCY;
        }
    } else if (dragState == WF_DRAG_RANGE) {
        float width = mouseTracker.getOriginDeltaMouseX();

        float pos;
        int last_type = mgr->getLastDemodulatorType();

        if (last_type == DEMOD_TYPE_LSB || last_type == DEMOD_TYPE_USB) {
            float pos1 = mouseTracker.getOriginMouseX();
            float pos2 = mouseTracker.getMouseX();

            if (pos2 < pos1) {
                float tmp = pos1;
                pos1 = pos2;
                pos2 = tmp;
            }

            pos = (last_type == DEMOD_TYPE_LSB)?pos2:pos1;
            width *= 2;
        } else {
            pos = mouseTracker.getOriginMouseX() + width / 2.0;
        }

        long long input_center_freq = getCenterFrequency();
        long long freq = input_center_freq - (long long) (0.5 * (float) getBandwidth()) + (long long) ((float) pos * (float) getBandwidth());
        unsigned int bw = (unsigned int) (fabs(width) * (float) getBandwidth());

        if (bw < MIN_BANDWIDTH) {
            bw = MIN_BANDWIDTH;
        }

        if (!bw) {
            dragState = WF_DRAG_NONE;
            return;
        }

        if (!isNew && wxGetApp().getDemodMgr().getDemodulators().size()) {
            demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
        } else {
            demod = wxGetApp().getDemodMgr().newThread();
            demod->setFrequency(freq);
            demod->setBandwidth(bw);

            demod->setDemodulatorType(mgr->getLastDemodulatorType());
            demod->setSquelchLevel(mgr->getLastSquelchLevel());
            demod->setSquelchEnabled(mgr->isLastSquelchEnabled());
            demod->setStereo(mgr->isLastStereo());
            demod->setGain(mgr->getLastGain());

            demod->run();

            wxGetApp().bindDemodulator(demod);
            wxGetApp().getDemodMgr().setActiveDemodulator(demod, false);
        }

        if (demod == NULL) {
            dragState = WF_DRAG_NONE;
            return;
        }

        setStatusText("New demodulator at frequency: %s", freq);

        wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getLastActiveDemodulator(), false);
        demod->updateLabel(freq);
        demod->setFrequency(freq);
        demod->setBandwidth(bw);
    }

    dragState = WF_DRAG_NONE;
}

void WaterfallCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
    wxGetApp().getDemodMgr().setActiveDemodulator(NULL);
    mouseZoom = 1.0;
}

void WaterfallCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
}

void WaterfallCanvas::OnMouseRightDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseRightDown(event);

    SetCursor(wxCURSOR_SIZENS);
    mouseTracker.setVertDragLock(true);
    mouseTracker.setHorizDragLock(true);
}

void WaterfallCanvas::OnMouseRightReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseRightReleased(event);
    SetCursor(wxCURSOR_CROSS);
    mouseTracker.setVertDragLock(false);
    mouseTracker.setHorizDragLock(false);
    mouseZoom = 1.0;
}

void WaterfallCanvas::attachWaterfallCanvas(WaterfallCanvas* canvas_in) {
    otherWaterfallCanvas = canvas_in;
    otherWaterfallCanvas->setPolling(false);
}


bool WaterfallCanvas::isPolling() {
    return polling;
}

void WaterfallCanvas::setPolling(bool polling) {
    this->polling = polling;
}
