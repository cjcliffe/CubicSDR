// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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
EVT_MOTION(SpectrumCanvas::OnMouseMoved)
EVT_LEFT_DOWN(SpectrumCanvas::OnMouseDown)
EVT_LEFT_UP(SpectrumCanvas::OnMouseReleased)
EVT_ENTER_WINDOW(SpectrumCanvas::OnMouseEnterWindow)
EVT_LEAVE_WINDOW(SpectrumCanvas::OnMouseLeftWindow)
EVT_MOUSEWHEEL(SpectrumCanvas::OnMouseWheelMoved)
EVT_RIGHT_DOWN(SpectrumCanvas::OnMouseRightDown)
EVT_RIGHT_UP(SpectrumCanvas::OnMouseRightReleased)
wxEND_EVENT_TABLE()

SpectrumCanvas::SpectrumCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs) :
        InteractiveCanvas(parent, dispAttrs), waterfallCanvas(NULL) {

    glContext = new PrimaryGLContext(this, &wxGetApp().GetContext(this), wxGetApp().GetContextAttributes());

    visualDataQueue->set_max_num_items(1);
            
    SetCursor(wxCURSOR_SIZEWE);
    scaleFactor = 1.0;
    resetScaleFactor = false;
    scaleFactorEnabled = false;
    bwChange = 0.0;
}

SpectrumCanvas::~SpectrumCanvas() {

}

void SpectrumCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();
    
    SpectrumVisualDataPtr vData;
    if (visualDataQueue->try_pop(vData)) {
            
        if (vData) {
            spectrumPanel.setPoints(vData->spectrum_points);
            spectrumPanel.setPeakPoints(vData->spectrum_hold_points);
            spectrumPanel.setFloorValue(vData->fft_floor);
            spectrumPanel.setCeilValue(vData->fft_ceiling);
        }
    }
    
    if (resetScaleFactor) {
        scaleFactor += (1.0-scaleFactor)*0.05;
        if (fabs(scaleFactor-1.0) < 0.01) {
            scaleFactor = 1.0;
            resetScaleFactor = false;
        }
        updateScaleFactor(scaleFactor);
    }
    
    
    glContext->SetCurrent(*this);
    initGLExtensions();

    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->BeginDraw(0,0,0);

    spectrumPanel.setFreq(getCenterFrequency());
    spectrumPanel.setBandwidth(getBandwidth());
    
    spectrumPanel.calcTransform(CubicVR::mat4::identity());
    spectrumPanel.draw();
    
    glLoadIdentity();
    
    auto demods = wxGetApp().getDemodMgr().getDemodulators();
    auto activeDemodulator = wxGetApp().getDemodMgr().getActiveContextModem();

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        if (!demods[i]->isActive()) {
            continue;
        }
        glContext->DrawDemodInfo(demods[i], ThemeMgr::mgr.currentTheme->fftHighlight, getCenterFrequency(), getBandwidth(), activeDemodulator==demods[i]);
    }

    if (waterfallCanvas && !activeDemodulator) {
        MouseTracker *wfmt = waterfallCanvas->getMouseTracker();
        if (wfmt->mouseInView()) {
            int snap = wxGetApp().getFrequencySnap();
            
            long long freq = getFrequencyAt(wfmt->getMouseX());
            
            if (snap > 1) {
                freq = roundf((float)freq/(float)snap)*snap;
            }

            auto lastActiveDemodulator = wxGetApp().getDemodMgr().getCurrentModem();

            bool isNew = (((waterfallCanvas->isShiftDown() || (lastActiveDemodulator && !lastActiveDemodulator->isActive())) && lastActiveDemodulator) || (!lastActiveDemodulator));
            
            glContext->DrawFreqBwInfo(freq, wxGetApp().getDemodMgr().getLastBandwidth(), isNew?ThemeMgr::mgr.currentTheme->waterfallNew:ThemeMgr::mgr.currentTheme->waterfallHover, getCenterFrequency(), getBandwidth(), true, true);
        }
    }
    
    glContext->EndDraw();

    spectrumPanel.drawChildren();

    SwapBuffers();
}


void SpectrumCanvas::OnIdle(wxIdleEvent &event) {
    Refresh();
    event.RequestMore();
}


void SpectrumCanvas::moveCenterFrequency(long long freqChange) {
    long long freq = wxGetApp().getFrequency();

    if (isView) {
        if (centerFreq - freqChange < bandwidth/2) {
            centerFreq = bandwidth/2;
        } else {
            centerFreq -= freqChange;
        }

        if (waterfallCanvas) {
            waterfallCanvas->setCenterFrequency(centerFreq);
        }

        long long bwOfs = (centerFreq > freq) ? ((long long) bandwidth / 2) : (-(long long) bandwidth / 2);
        long long freqEdge = centerFreq + bwOfs;

        if (abs(freq - freqEdge) > (wxGetApp().getSampleRate() / 2)) {
            freqChange = -((centerFreq > freq) ? (freqEdge - freq - (wxGetApp().getSampleRate() / 2)) : (freqEdge - freq + (wxGetApp().getSampleRate() / 2)));
        } else {
            freqChange = 0;
        }
    }

    if (freqChange) {
        if (freq - freqChange < wxGetApp().getSampleRate()/2) {
            freq = wxGetApp().getSampleRate()/2;
        } else {
            freq -= freqChange;
        }
        wxGetApp().setFrequency(freq);
    }
}

void SpectrumCanvas::setShowDb(bool showDb) {
    spectrumPanel.setShowDb(showDb);
}

bool SpectrumCanvas::getShowDb() {
    return spectrumPanel.getShowDb();
}

void SpectrumCanvas::setUseDBOfs(bool showDb) {
    spectrumPanel.setUseDBOffset(showDb);
}

bool SpectrumCanvas::getUseDBOfs() {
    return spectrumPanel.getUseDBOffset();
}

void SpectrumCanvas::setView(long long center_freq_in, int bandwidth_in) {
    bwChange += bandwidth_in-bandwidth;
    #define BW_RESET_TH 400000
    if (bwChange > BW_RESET_TH || bwChange < -BW_RESET_TH) {
        resetScaleFactor = true;
        bwChange = 0;
    }
    InteractiveCanvas::setView(center_freq_in, bandwidth_in);
}

void SpectrumCanvas::disableView() {
    InteractiveCanvas::disableView();
}

void SpectrumCanvas::setScaleFactorEnabled(bool en) {
    scaleFactorEnabled = en;
}

void SpectrumCanvas::setFFTSize(int fftSize) {
    spectrumPanel.setFFTSize(fftSize);
}

void SpectrumCanvas::updateScaleFactor(float factor) {
    SpectrumVisualProcessor *sp = wxGetApp().getSpectrumProcessor();
    FFTVisualDataThread *wdt = wxGetApp().getAppFrame()->getWaterfallDataThread();
    SpectrumVisualProcessor *wp = wdt->getProcessor();

    scaleFactor = factor;
    sp->setScaleFactor(factor);
    wp->setScaleFactor(factor);
}

void SpectrumCanvas::updateScaleFactorFromYMove(float yDeltaMouseMove) {

    scaleFactor += yDeltaMouseMove * 2.0;

    if (scaleFactor < 0.25) {
        scaleFactor = 0.25;
    }
    if (scaleFactor > 10.0) {
        scaleFactor = 10.0;
    }

    resetScaleFactor = false;
    updateScaleFactor(scaleFactor);
}

void SpectrumCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);
    if (mouseTracker.mouseDown()) {
        int freqChange = mouseTracker.getDeltaMouseX() * getBandwidth();

        if (freqChange != 0) {
            moveCenterFrequency(freqChange);
        }
    }
    else if (scaleFactorEnabled && mouseTracker.mouseRightDown()) {
        
        updateScaleFactorFromYMove(mouseTracker.getDeltaMouseY());

    } else {
        if (scaleFactorEnabled) {
            setStatusText("Drag horizontal to adjust center frequency. Arrow keys or wheel to navigate/zoom bandwith. Right-drag or SHIFT+UP/DOWN to adjust visual gain, right-click to reset it. 'B' to toggle decibels display.");
        } else {
            setStatusText("Displaying spectrum of active demodulator.");
        }
    }
}

void SpectrumCanvas::OnMouseDown(wxMouseEvent& event) {
    SetCursor(wxCURSOR_SIZEWE);
	mouseTracker.setVertDragLock(true);
    InteractiveCanvas::OnMouseDown(event);
}

void SpectrumCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
    if (waterfallCanvas) {
        waterfallCanvas->OnMouseWheelMoved(event);
    }
}

void SpectrumCanvas::OnMouseReleased(wxMouseEvent& event) {
	mouseTracker.setVertDragLock(false);
	InteractiveCanvas::OnMouseReleased(event);
    SetCursor(wxCURSOR_CROSS);
}

void SpectrumCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
#ifdef _WIN32
    if (wxGetApp().getAppFrame()->canFocus()) {
        this->SetFocus();
    }
#endif
}

void SpectrumCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
}

void SpectrumCanvas::attachWaterfallCanvas(WaterfallCanvas* canvas_in) {
    waterfallCanvas = canvas_in;
}

SpectrumVisualDataQueuePtr SpectrumCanvas::getVisualDataQueue() {
    return visualDataQueue;
}

void SpectrumCanvas::OnMouseRightDown(wxMouseEvent& event) {
    SetCursor(wxCURSOR_SIZENS);
	mouseTracker.setHorizDragLock(true);
    mouseTracker.OnMouseRightDown(event);
    scaleFactor = wxGetApp().getSpectrumProcessor()->getScaleFactor();
}

void SpectrumCanvas::OnMouseRightReleased(wxMouseEvent& event) {
    SetCursor(wxCURSOR_CROSS);
	mouseTracker.setHorizDragLock(false);
    
    if (!mouseTracker.getOriginDeltaMouseY()) {
        resetScaleFactor = true;

        wxGetApp().getSpectrumProcessor()->setPeakHold(wxGetApp().getSpectrumProcessor()->getPeakHold());

        //make the peak hold act on the current dmod also, like a zoomed-in version.
        if (wxGetApp().getDemodSpectrumProcessor()) {
            wxGetApp().getDemodSpectrumProcessor()->setPeakHold(wxGetApp().getSpectrumProcessor()->getPeakHold());
        }
    }

    mouseTracker.OnMouseRightReleased(event);
}

void SpectrumCanvas::OnKeyDown(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyDown(event);

    switch (event.GetKeyCode()) {
  
    case 'B':
       setShowDb(!getShowDb());
       break;
    default:
        event.Skip();
    }
}

void SpectrumCanvas::OnKeyUp(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyUp(event);
}
