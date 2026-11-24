// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "TuningCanvas.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include "TuneRange.h"
#include "AppFrame.h"
#include "TunerStep.h"
#include <algorithm>

wxBEGIN_EVENT_TABLE(TuningCanvas, wxGLCanvas) EVT_PAINT(TuningCanvas::OnPaint)
EVT_MOTION(TuningCanvas::OnMouseMoved)
EVT_LEFT_DOWN(TuningCanvas::OnMouseDown)
EVT_LEFT_UP(TuningCanvas::OnMouseReleased)
EVT_RIGHT_DOWN(TuningCanvas::OnMouseRightDown)
EVT_RIGHT_UP(TuningCanvas::OnMouseRightReleased)

EVT_LEAVE_WINDOW(TuningCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(TuningCanvas::OnMouseEnterWindow)
EVT_MOUSEWHEEL(TuningCanvas::OnMouseWheelMoved)
//EVT_KEY_DOWN(TuningCanvas::OnKeyDown)
//EVT_KEY_UP(TuningCanvas::OnKeyUp)
wxEND_EVENT_TABLE()

TuningCanvas::TuningCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs) :
        InteractiveCanvas(parent, dispAttrs), dragAccum(0), uxDown(0), top(false), bottom(false), freq(-1), bw(-1), center(-1), halfBand(false) {

    glContext = new TuningContext(this, &wxGetApp().GetContext(this), wxGetApp().GetContextAttributes());

    hoverIndex = 0;
    downIndex = 0;
    hoverState = TUNING_HOVER_NONE;
    downState = TUNING_HOVER_NONE;
    dragging = false;

    freqDP = -1.0;
    freqW = (1.0f / 3.0f) * 2.0f;

    bwDP = -1.0 + (2.25 / 3.0);
    bwW = (1.0 / 4.0) * 2.0;

    centerDP = -1.0f + (2.0f / 3.0f) * 2.0f;
    centerW = (1.0f / 3.0f) * 2.0f;

    currentPPM = 0;
}

TuningCanvas::~TuningCanvas() = default;

void TuningCanvas::setHalfBand(bool hb) {
    halfBand = hb;
	Refresh();
}

void TuningCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
 //   wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize() * GetContentScaleFactor();
    
    glContext->SetCurrent(*this);
    initGLExtensions();
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();

    auto activeDemod = wxGetApp().getDemodMgr().getCurrentModem();
    
    freq = 0;
    if (activeDemod != nullptr) {
        freq = activeDemod->getFrequency();
    }
    bw = wxGetApp().getDemodMgr().getLastBandwidth();
    center = wxGetApp().getFrequency();

    if (mouseTracker.mouseDown()) {
        glContext->Draw(ThemeMgr::mgr.currentTheme->tuningBarDark.r, ThemeMgr::mgr.currentTheme->tuningBarDark.g, ThemeMgr::mgr.currentTheme->tuningBarDark.b,
                0.75, mouseTracker.getOriginMouseX(), mouseTracker.getMouseX());
    }

    RGBA4f clr = top ? ThemeMgr::mgr.currentTheme->tuningBarUp : ThemeMgr::mgr.currentTheme->tuningBarDown;

    RGBA4f clrDark = ThemeMgr::mgr.currentTheme->tuningBarDark;
    RGBA4f clrMid = ThemeMgr::mgr.currentTheme->tuningBarLight;

    glContext->DrawTunerBarIndexed(1, 3, 11, freqDP, freqW, clrMid, 0.25, true, true); // freq
    glContext->DrawTunerBarIndexed(4, 6, 11, freqDP, freqW, clrDark, 0.25, true, true);
    glContext->DrawTunerBarIndexed(7, 9, 11, freqDP, freqW, clrMid, 0.25, true, true);
    glContext->DrawTunerBarIndexed(10, 11, 11, freqDP, freqW, clrDark, 0.25, true, true);

    glContext->DrawTunerBarIndexed(1, 3, 7, bwDP, bwW, clrMid, 0.25, true, true); // bw
    glContext->DrawTunerBarIndexed(4, 6, 7, bwDP, bwW, clrDark, 0.25, true, true);
    glContext->DrawTunerBarIndexed(7, 7, 7, bwDP, bwW, clrMid, 0.25, true, true);

    glContext->DrawTunerBarIndexed(1, 3, 11, centerDP, centerW, clrMid, 0.25, true, true); // freq
    glContext->DrawTunerBarIndexed(4, 6, 11, centerDP, centerW, clrDark, 0.25, true, true);
    glContext->DrawTunerBarIndexed(7, 9, 11, centerDP, centerW, clrMid, 0.25, true, true);
    glContext->DrawTunerBarIndexed(10, 11, 11, centerDP, centerW, clrDark, 0.25, true, true);

    if (hoverIndex > 0 && !mouseTracker.mouseDown()) {
        switch (hoverState) {

        case TUNING_HOVER_FREQ:
            glContext->DrawTunerBarIndexed(hoverIndex, hoverIndex, 11, freqDP, freqW, clr, 0.25, top, bottom); // freq
            break;
        case TUNING_HOVER_BW:
            glContext->DrawTunerBarIndexed(hoverIndex, hoverIndex, 7, bwDP, bwW, clr, 0.25, top, bottom); // bw
            break;
        case TUNING_HOVER_CENTER:
            glContext->DrawTunerBarIndexed(hoverIndex, hoverIndex, 11, centerDP, centerW, clr, 0.25, top, bottom); // center
            break;
        case TUNING_HOVER_NONE:
            break;
        case TUNING_HOVER_PPM:
            glContext->DrawTunerBarIndexed(hoverIndex, hoverIndex, 11, freqDP, freqW, clr, 0.25, top, bottom); // freq
            break;
         }
    }

    if (altDown) {
        glContext->DrawTuner(currentPPM, 11, freqDP, freqW);
    } else {
        glContext->DrawTuner(freq, 11, freqDP, freqW);
        int snap = wxGetApp().getFrequencySnap();
        if (snap != 1) {
            glContext->DrawTunerDigitBox((int)log10(snap), 11, freqDP, freqW, RGBA4f(1.0,0.0,0.0));
        }
    }
    glContext->DrawTuner(halfBand?(bw/2):bw, 7, bwDP, bwW);
    glContext->DrawTuner(center, 11, centerDP, centerW);

    glContext->DrawEnd();

    SwapBuffers();
}

/***
 * Perform tuner step
 *
 * @param state     Current hover state
 * @param digit     Digit position
 * @param tuningDir        Tuning direction, true for up
 * @param preventCarry Prevent carry operation on digit overflow
 * @param zeroOut   Zero out 'digit' and lower digits
 */
void TuningCanvas::StepTuner(ActiveState state, TuningDirection tuningDir, int digit, bool preventCarry, bool zeroOut) {
    auto activeDemod = wxGetApp().getDemodMgr().getCurrentModem();
    if (state == TUNING_HOVER_FREQ && activeDemod) {
        long long demod_freq = activeDemod->getFrequency();
        long long diff = abs(wxGetApp().getFrequency() - demod_freq);

        TunerStep::StepInput step;
        step.value = demod_freq;
        step.digit = digit;
        step.up = (tuningDir == TUNING_DIRECTION_UP);
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.bottom = bottom;
        demod_freq = TunerStep::stepDigit(step);

        //Pin the stepped frequency into the view like a drag does; past the
        //data window the post thread would deactivate the demod.
        AppFrame *appframe = wxGetApp().getAppFrame();
        if (appframe && appframe->getWaterfallCanvas()) {
            demod_freq = pinDemodCenter(demod_freq, appframe->getWaterfallCanvas()->getCenterFrequency(),
                appframe->getWaterfallCanvas()->getBandwidth());
        }

        if (wxGetApp().getSampleRate() / 2 < diff) {
            wxGetApp().setFrequency(demod_freq);
        }

        activeDemod->setTracking(true);
        activeDemod->setFollow(true);
        activeDemod->setFrequency(demod_freq);
        if (activeDemod->isDeltaLock()) {
            activeDemod->setDeltaLockOfs(deltaLockOffset(activeDemod->getFrequency(), wxGetApp().getFrequency()));
        }
        activeDemod->updateLabel(demod_freq);
    }

    if (state == TUNING_HOVER_BW) {
        long nbw = wxGetApp().getDemodMgr().getLastBandwidth();

        TunerStep::StepInput step;
        step.value = nbw;
        step.digit = digit;
        step.up = (tuningDir == TUNING_DIRECTION_UP);
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.doubleAmount = halfBand;
        step.bottom = bottom;
        nbw = (long)TunerStep::stepDigit(step);

        if (nbw > CHANNELIZER_RATE_MAX) {
            nbw = CHANNELIZER_RATE_MAX;
        }

        wxGetApp().getDemodMgr().setLastBandwidth(nbw);

        if (activeDemod) {
            activeDemod->setBandwidth(wxGetApp().getDemodMgr().getLastBandwidth());
        }
    }

    if (state == TUNING_HOVER_CENTER) {
        long long ctr = wxGetApp().getFrequency();
        TunerStep::StepInput step;
        step.value = ctr;
        step.digit = digit;
        step.up = (tuningDir == TUNING_DIRECTION_UP);
        step.preventCarry = preventCarry;
        step.zeroOut = zeroOut;
        step.bottom = bottom;
        ctr = TunerStep::stepDigit(step);

        wxGetApp().setFrequency(ctr);
    }

    if (state == TUNING_HOVER_PPM) {
        TunerStep::StepInput step;
        step.value = currentPPM;
        step.digit = digit;
        step.up = (tuningDir == TUNING_DIRECTION_UP);
        step.preventCarry = preventCarry;
        step.bottom = bottom;
        currentPPM = (int)TunerStep::stepDigit(step);

        if (currentPPM > 2000) {
            currentPPM = 2000;
        }

        if (currentPPM < -2000) {
            currentPPM = -2000;
        }

        wxGetApp().setPPM(currentPPM);
        wxGetApp().notifyMainUIOfDeviceChange();
    }
}

void TuningCanvas::frameUpdate() {
    if (mouseTracker.mouseDown()) {
        if (downState != TUNING_HOVER_NONE) {
            dragAccum += 5.0*mouseTracker.getOriginDeltaMouseX();
            TunerStep::DragSteps drag = TunerStep::dragSteps(dragAccum);
            dragAccum = drag.rest;
            for (int i = 0; i < drag.up; i++) {
                StepTuner(downState, TUNING_DIRECTION_UP, downIndex - 1, shiftDown, false);
                dragging = true;
            }
            for (int i = 0; i < drag.down; i++) {
                StepTuner(downState, TUNING_DIRECTION_DOWN, downIndex - 1, shiftDown, false);
                dragging = true;
            }
        } else {
            dragAccum = 0;
            dragging = false;
        }
    }
    // Painted values have no event path for external tunes: repaint on change.
    long long liveFreq = 0;
    auto liveDemod = wxGetApp().getDemodMgr().getCurrentModem();
    if (liveDemod != nullptr) {
        liveFreq = liveDemod->getFrequency();
    }
    long long liveBw = wxGetApp().getDemodMgr().getLastBandwidth();
    long long liveCenter = wxGetApp().getFrequency();
    if (firstPaintCheck || liveFreq != paintedFreq || liveBw != paintedBw || liveCenter != paintedCenter
        || currentPPM != paintedPPM || altDown != paintedAlt || shiftDown != paintedShift) {
        paintedFreq = liveFreq;
        paintedBw = liveBw;
        paintedCenter = liveCenter;
        paintedPPM = currentPPM;
        paintedAlt = altDown;
        paintedShift = shiftDown;
        firstPaintCheck = false;
        Refresh();
    }
}

void TuningCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);

    top = mouseTracker.getMouseY() >= 0.5;
    bottom = mouseTracker.getMouseY() <= 0.5;

    int index = glContext->GetTunerDigitIndex(mouseTracker.getMouseX(), 11, freqDP, freqW); // freq
    if (index > 0) {
        hoverIndex = index;
        hoverState = altDown?TUNING_HOVER_PPM:TUNING_HOVER_FREQ;
    }

    if (!index) {
        index = glContext->GetTunerDigitIndex(mouseTracker.getMouseX(), 7, bwDP, bwW); // bw
        if (index > 0) {
            hoverIndex = index;
            hoverState = TUNING_HOVER_BW;
        }
    }

    if (!index) {
        index = glContext->GetTunerDigitIndex(mouseTracker.getMouseX(), 11, centerDP, centerW); // center
        if (index > 0) {
            hoverIndex = index;
            hoverState = TUNING_HOVER_CENTER;
        }
    }

    if (!index) {
        hoverIndex = 0;
        hoverState = TUNING_HOVER_NONE;
    } else {
        switch (hoverState) {
        case TUNING_HOVER_FREQ:
                setStatusText("Click, wheel or drag(left/right) a digit to change frequency; SPACE for direct input. Right click to set/clear snap. Hold ALT to change PPM. Hold SHIFT to disable carry. SHIFT-right click to Zero Right.");
            break;
        case TUNING_HOVER_BW:
                setStatusText("Click, wheel or drag(left/right) a digit to change bandwidth; SPACE for direct input.  Hold SHIFT to disable carry. SHIFT-right click to Zero Right.");
            break;
        case TUNING_HOVER_CENTER:
                setStatusText("Click, wheel or drag(left/right) a digit to change center frequency; SPACE for direct input.  Hold SHIFT to disable carry. SHIFT-right click to Zero Right.");
            break;
        case TUNING_HOVER_PPM:
                 setStatusText("Click, wheel or drag(left/right) a digit to change device PPM offset.  Hold SHIFT to disable carry.");
             break;
        case TUNING_HOVER_NONE:
            setStatusText("");
            break;
      }
    }

    Refresh();
}

void TuningCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);

    uxDown = 2.0 * (mouseTracker.getMouseX() - 0.5);
    dragAccum = 0;

    mouseTracker.setVertDragLock(true);
    downIndex = hoverIndex;
    downState = hoverState;
    Refresh();
}

void TuningCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);

    int hExponent = hoverIndex - 1;

    if (hoverState != TUNING_HOVER_NONE && !mouseTracker.mouseDown() && hoverIndex) {
        bool vertical = (event.m_wheelAxis == wxMOUSE_WHEEL_VERTICAL);
        int *accum = vertical ? &wheelAccumV : &wheelAccumH;
        int delta = event.m_wheelDelta ? event.m_wheelDelta : 120;
        TunerStep::WheelSteps wheel = TunerStep::wheelSteps(*accum, event.m_wheelRotation, delta);
        *accum = wheel.rest;
        int steps = wheel.steps;
        if (steps != 0) {
            TuningDirection dir;
            if (vertical) {
                dir = (steps > 0) ? TUNING_DIRECTION_UP : TUNING_DIRECTION_DOWN;
            } else {
                dir = (steps < 0) ? TUNING_DIRECTION_UP : TUNING_DIRECTION_DOWN;
            }
            for (int i = 0, n = steps > 0 ? steps : -steps; i < n; i++) {
                StepTuner(hoverState, dir, hExponent, shiftDown, false);
            }
        }
    }
    Refresh();
}

void TuningCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);

    int hExponent = hoverIndex - 1;

    if (hoverState != TUNING_HOVER_NONE && !dragging && (downState == hoverState) && (downIndex == hoverIndex)) {
        StepTuner(hoverState, top ? TUNING_DIRECTION_UP : TUNING_DIRECTION_DOWN, hExponent, shiftDown, false);
    }

    mouseTracker.setVertDragLock(false);

    dragging = false;
    SetCursor(wxCURSOR_ARROW);
    Refresh();
}

void TuningCanvas::OnMouseRightDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseRightDown(event);

    uxDown = 2.0 * (mouseTracker.getMouseX() - 0.5);

    downIndex = hoverIndex;
    downState = hoverState;
}

void TuningCanvas::OnMouseRightReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseRightReleased(event);

    if (shiftDown) {
        int hExponent = hoverIndex - 1;

        if (hoverState != TUNING_HOVER_NONE && !dragging && (downState == hoverState) && (downIndex == hoverIndex)) {
            StepTuner(hoverState, top ? TUNING_DIRECTION_UP : TUNING_DIRECTION_DOWN, hExponent, false, true);
        }
    } else if (hoverState == TUNING_HOVER_FREQ) {
        if (hoverIndex == 1) {
            wxGetApp().setFrequencySnap(1);
        } else if (hoverIndex > 1 && hoverIndex < 8) {
            int exp = pow(10, hoverIndex-1);
            if (wxGetApp().getFrequencySnap() == exp) {
                wxGetApp().setFrequencySnap(1);
            } else {
                wxGetApp().setFrequencySnap(exp);
            }
        }
    }
    Refresh();
}

void TuningCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
    hoverIndex = 0;
    hoverState = TUNING_HOVER_NONE;
    Refresh();
}

void TuningCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_ARROW);
    hoverIndex = 0;
    hoverState = TUNING_HOVER_NONE;
    currentPPM = wxGetApp().getPPM();
#ifdef _WIN32
	if (wxGetApp().getAppFrame()->canFocus()) {
		this->SetFocus();
	}
#endif
    Refresh();
}

void TuningCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

void TuningCanvas::OnKeyDown(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyDown(event);

    if (event.GetKeyCode() == WXK_SPACE) {
        if (hoverState == TUNING_HOVER_CENTER || hoverState == TUNING_HOVER_FREQ) {
            wxGetApp().showFrequencyInput(FrequencyDialog::FDIALOG_TARGET_DEFAULT);
        } else if (hoverState == TUNING_HOVER_BW) {
            wxGetApp().showFrequencyInput(FrequencyDialog::FDIALOG_TARGET_BANDWIDTH);
        }
    } 
}

void TuningCanvas::OnKeyUp(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyUp(event);
}

TuningCanvas::ActiveState TuningCanvas::getHoverState() {
    return hoverState;
}
