// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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

#ifdef USE_HAMLIB
#include "RigThread.h"
#endif

#include <wx/numformatter.h>

#include "DemodulatorThread.h"

wxBEGIN_EVENT_TABLE(WaterfallCanvas, wxGLCanvas)
EVT_PAINT(WaterfallCanvas::OnPaint)
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

WaterfallCanvas::WaterfallCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs) :
        InteractiveCanvas(parent, dispAttrs), dragState(WF_DRAG_NONE), nextDragState(WF_DRAG_NONE), fft_size(0), new_fft_size(0), waterfall_lines(0),
        dragOfs(0), mouseZoom(1), zoom(1), freqMoving(false), freqMove(0.0), hoverAlpha(1.0) {

    glContext = new PrimaryGLContext(this, &wxGetApp().GetContext(this), wxGetApp().GetContextAttributes());
    linesPerSecond = DEFAULT_WATERFALL_LPS;
    lpsIndex = 0;
    preBuf = false;
    SetCursor(wxCURSOR_CROSS);
    scaleMove = 0;
    minBandwidth = 30000;
    fft_size_changed.store(false);
}

WaterfallCanvas::~WaterfallCanvas() {
}

void WaterfallCanvas::setup(unsigned int fft_size_in, int waterfall_lines_in) {
    if (fft_size == fft_size_in && waterfall_lines_in == waterfall_lines) {
        return;
    }
    fft_size = fft_size_in;
    waterfall_lines = waterfall_lines_in;

    waterfallPanel.setup(fft_size, waterfall_lines);
    gTimer.start();
}

void WaterfallCanvas::setFFTSize(unsigned int fft_size_in) {
    if (fft_size_in == fft_size) {
        return;
    }
    new_fft_size = fft_size_in;
    fft_size_changed.store(true);
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

void WaterfallCanvas::processInputQueue() {
    std::lock_guard < std::mutex > lock(tex_update);
    
    gTimer.update();
    
    double targetVis =  1.0 / (double)linesPerSecond;
    lpsIndex += gTimer.lastUpdateSeconds();

    bool updated = false;
    if (linesPerSecond) {
        if (lpsIndex >= targetVis) {
            while (lpsIndex >= targetVis) {
                SpectrumVisualDataPtr vData;

                if (visualDataQueue->try_pop(vData)) {
                    
                    if (vData) {
                        if (vData->spectrum_points.size() == fft_size * 2) {
                            waterfallPanel.setPoints(vData->spectrum_points);
                        }
                        waterfallPanel.step();
                      
                        updated = true;
                    }
                    lpsIndex-=targetVis;
                } else {
                	break;
                }
            }
        }
    }
    if (updated) {
        wxClientDC(this);
        glContext->SetCurrent(*this);
        waterfallPanel.update();
    }
   
}

void WaterfallCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    std::lock_guard < std::mutex > lock(tex_update);
//    wxPaintDC dc(this);
#ifdef USE_OSX_RETINA
    const wxSize ClientSize = GetClientSize() * GetContentScaleFactor();
#else
    const wxSize ClientSize = GetClientSize();
#endif

    long double currentZoom = zoom;
    
    if (mouseZoom != 1) {
        currentZoom = mouseZoom;
        mouseZoom = mouseZoom + (1.0 - mouseZoom) * 0.2;
        if (fabs(mouseZoom-1.0)<0.01) {
            mouseZoom = 1;
        }
    }
    
    if (scaleMove != 0) {
        SpectrumVisualProcessor *sp = wxGetApp().getSpectrumProcessor();
        FFTVisualDataThread *wdt = wxGetApp().getAppFrame()->getWaterfallDataThread();
        SpectrumVisualProcessor *wp = wdt->getProcessor();
        float factor = sp->getScaleFactor();

        factor += scaleMove * 0.02;
        
        if (factor < 0.25) {
            factor = 0.25;
        }
        if (factor > 10.0) {
            factor = 10.0;
        }
        
        sp->setScaleFactor(factor);
        wp->setScaleFactor(factor);
    }
    
    if (freqMove != 0.0) {
        long long newFreq = getCenterFrequency() + (long long)((long double)getBandwidth()*freqMove) * 0.01;
        
        long long minFreq = bandwidth/2;
        if (newFreq < minFreq) {
            newFreq = minFreq;
        }

        updateCenterFrequency(newFreq);
        
        if (!freqMoving) {
            freqMove -= (freqMove * 0.2);
            if (fabs(freqMove) < 0.01) {
                freqMove = 0.0;
            }
        }
    }
    
    long long bw;
    if (currentZoom != 1) {
        long long freq = wxGetApp().getFrequency();
        bw = getBandwidth();

        double mpos = 0;
        float mouseInView = false;

        if (mouseTracker.mouseInView()) {
            mpos = mouseTracker.getMouseX();
            mouseInView = true;
        } else if (spectrumCanvas && spectrumCanvas->getMouseTracker()->mouseInView()) {
            mpos = spectrumCanvas->getMouseTracker()->getMouseX();
            mouseInView = true;
        }

        if (currentZoom < 1) {
            bw = (long long) ceil((long double) bw * currentZoom);
            if (bw < minBandwidth) {
                bw = minBandwidth;
            }
            if (mouseInView) {
                long long mfreqA = getFrequencyAt(mpos, centerFreq, getBandwidth());
                long long mfreqB = getFrequencyAt(mpos, centerFreq, bw);
                centerFreq += mfreqA - mfreqB;
            }
            
            setView(centerFreq, bw);
        } else {
            if (isView) {
                bw = (long long) ceil((long double) bw * currentZoom);

                if (bw >= wxGetApp().getSampleRate()) {
                    disableView();
                    if (spectrumCanvas) {
                        spectrumCanvas->disableView();
                    }
                    bw = wxGetApp().getSampleRate();
                    centerFreq = wxGetApp().getFrequency();
                } else {
                    if (mouseInView) {
                        long long mfreqA = getFrequencyAt(mpos, centerFreq, getBandwidth());
                        long long mfreqB = getFrequencyAt(mpos, centerFreq, bw);
                        centerFreq += mfreqA - mfreqB;
                        setBandwidth(bw);
                    } else {
                        setBandwidth(bw);
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

        if (spectrumCanvas) {
            if ((spectrumCanvas->getCenterFrequency() != centerFreq) || (spectrumCanvas->getBandwidth() != bw)) {
                if (getViewState()) {
                    spectrumCanvas->setView(centerFreq,bw);
                } else {
                    spectrumCanvas->disableView();
                    spectrumCanvas->setCenterFrequency(centerFreq);
                    spectrumCanvas->setBandwidth(bw);
                }
            }
        }
    }
    

    glContext->SetCurrent(*this);
    initGLExtensions();
    glViewport(0, 0, ClientSize.x, ClientSize.y);
    
    if (fft_size_changed.load()) {
        fft_size = new_fft_size;
        waterfallPanel.setup(fft_size, waterfall_lines);
        fft_size_changed.store(false);
    }

    glContext->BeginDraw(0,0,0);

    waterfallPanel.calcTransform(CubicVR::mat4::identity());
    waterfallPanel.draw();

    auto demods = wxGetApp().getDemodMgr().getDemodulators();

    auto activeDemodulator = wxGetApp().getDemodMgr().getActiveContextModem();
    auto lastActiveDemodulator = wxGetApp().getDemodMgr().getCurrentModem();

    bool isNew = shiftDown
            || (wxGetApp().getDemodMgr().getCurrentModem() && !wxGetApp().getDemodMgr().getCurrentModem()->isActive());

    int currentBandwidth = getBandwidth();
    long long currentCenterFreq = getCenterFrequency();

    ColorTheme *currentTheme = ThemeMgr::mgr.currentTheme;
    std::string last_type = wxGetApp().getDemodMgr().getLastDemodulatorType();

    if (mouseTracker.mouseInView() || wxGetApp().getDemodMgr().getActiveContextModem()) {
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

            if ((last_type == "LSB" || last_type == "USB") && mouseTracker.mouseDown()) {
                centerPos = mouseTracker.getMouseX();
                glContext->DrawRangeSelector(centerPos, centerPos-width, isNew?currentTheme->waterfallNew:currentTheme->waterfallHover);
            } else {
                glContext->DrawFreqSelector(centerPos, isNew?currentTheme->waterfallNew:currentTheme->waterfallHover, width, currentCenterFreq, currentBandwidth);
            }
        } else {
            if (lastActiveDemodulator) {
                glContext->DrawDemod(lastActiveDemodulator, ((isNew && activeDemodulator == nullptr) || (activeDemodulator != nullptr))?currentTheme->waterfallHighlight:currentTheme->waterfallDestroy, currentCenterFreq, currentBandwidth);
            }
            if (activeDemodulator == nullptr) {
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

    glContext->setHoverAlpha(0);

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        if (!demods[i]->isActive()) {
            continue;
        }
        if (activeDemodulator == demods[i] || lastActiveDemodulator == demods[i]) {
            continue;
        }
        glContext->DrawDemod(demods[i], currentTheme->waterfallHighlight, currentCenterFreq, currentBandwidth);
    }

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        demods[i]->getVisualCue()->step();

        int squelchBreak = demods[i]->getVisualCue()->getSquelchBreak();
        if (squelchBreak) {
            glContext->setHoverAlpha((float(squelchBreak) / 60.0));
            glContext->DrawDemod(demods[i], currentTheme->waterfallHover, currentCenterFreq, currentBandwidth);
        }
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
    case WXK_UP:
    case WXK_NUMPAD_UP:
            scaleMove = 0.0;
            zoom = 1.0;
            if (mouseZoom != 1.0) {
                mouseZoom = 0.95f;
            }
        break;
    case WXK_DOWN:
    case WXK_NUMPAD_DOWN:
            scaleMove = 0.0;
            zoom = 1.0;
            if (mouseZoom != 1.0) {
                mouseZoom = 1.05f;
            }
        break;
    case WXK_LEFT:
    case WXK_NUMPAD_LEFT:
    case WXK_RIGHT:
    case WXK_NUMPAD_RIGHT:
            freqMoving = false;
            break;
    }
}

void WaterfallCanvas::OnKeyDown(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyDown(event);

    auto activeDemod = wxGetApp().getDemodMgr().getActiveContextModem();

    long long originalFreq = getCenterFrequency();
    long long freq = originalFreq;

    switch (event.GetKeyCode()) {
    case WXK_UP:
    case WXK_NUMPAD_UP:
            if (!shiftDown) {
                mouseZoom = 1.0;
                zoom = 0.95f;
            } else {
                scaleMove = 1.0;
            }
        break;
    case WXK_DOWN:
    case WXK_NUMPAD_DOWN:
            if (!shiftDown) {
                mouseZoom = 1.0;
                zoom = 1.05f;
            } else {
                scaleMove = -1.0;
            }
        break;
    case WXK_RIGHT:
    case WXK_NUMPAD_RIGHT:
        if (isView) {
            freqMove = shiftDown?5.0:1.0;
            freqMoving = true;
        } else {
            freq += shiftDown?(getBandwidth() * 10):(getBandwidth() / 2);
        }
        break;
    case WXK_LEFT:
    case WXK_NUMPAD_LEFT:
        if (isView) {
            freqMove = shiftDown?-5.0:-1.0;
            freqMoving = true;
        } else {
            freq -= shiftDown?(getBandwidth() * 10):(getBandwidth() / 2);
        }
        break;
    case 'D':
    case WXK_DELETE:
        if (!activeDemod) {
            break;
        }
        wxGetApp().removeDemodulator(activeDemod);
        wxGetApp().getDemodMgr().deleteThread(activeDemod);
        break;
    case WXK_SPACE:
        wxGetApp().showFrequencyInput();
        break;
    case 'E': //E is for 'Edit the label' of the active demodulator. 
        wxGetApp().showLabelInput();
        break;
    case 'C':
        if (wxGetApp().getDemodMgr().getActiveContextModem()) {
            wxGetApp().setFrequency(wxGetApp().getDemodMgr().getActiveContextModem()->getFrequency());
        } else if (mouseTracker.mouseInView()) {
            long long freq = getFrequencyAt(mouseTracker.getMouseX());
            
            int snap = wxGetApp().getFrequencySnap();
            
            if (snap > 1) {
                freq = roundf((float)freq/(float)snap)*snap;
            }
            
            wxGetApp().setFrequency(freq);
        }
#ifdef USE_HAMLIB
        if (wxGetApp().rigIsActive() && (!wxGetApp().getRigThread()->getControlMode() || wxGetApp().getRigThread()->getCenterLock())) {
            wxGetApp().getRigThread()->setFrequency(wxGetApp().getFrequency(),true);
        }
#endif
        break;
    default:
        event.Skip();
        return;
    }

    long long minFreq = bandwidth/2;
    if (freq < minFreq) {
        freq = minFreq;
    }

    if (freq != originalFreq) {
        updateCenterFrequency(freq);
    }

}
void WaterfallCanvas::OnIdle(wxIdleEvent & /* event */) {
    processInputQueue();
    Refresh();
}

void WaterfallCanvas::updateHoverState() {
    long long freqPos = getFrequencyAt(mouseTracker.getMouseX());
    
    auto demodsHover = wxGetApp().getDemodMgr().getDemodulatorsAt(freqPos, 15000);
    
    wxGetApp().getDemodMgr().setActiveDemodulator(nullptr);
    
    if (altDown) {
        nextDragState = WF_DRAG_RANGE;
        mouseTracker.setVertDragLock(true);
        mouseTracker.setHorizDragLock(false);
        if (shiftDown) {
            setStatusText("Click and drag to create a new demodulator by range.");
        } else {
            setStatusText("Click and drag to set the current demodulator range.");
        }
    } else if (demodsHover.size() && !shiftDown) {
        long near_dist = getBandwidth();
        
        DemodulatorInstancePtr activeDemodulator = nullptr;
        
        for (int i = 0, iMax = demodsHover.size(); i < iMax; i++) {
            auto demod = demodsHover[i];
            long long freqDiff = demod->getFrequency() - freqPos;
            long halfBw = (demod->getBandwidth() / 2);
            long long currentBw = getBandwidth();
            long long globalBw = wxGetApp().getSampleRate();
            long dist = abs(freqDiff);
            double bufferBw = 10000.0 * ((double)currentBw / (double)globalBw);
            double maxDist = ((double)halfBw + bufferBw);
            
            if ((double)dist <= maxDist) {
                if ((freqDiff > 0 && demod->getDemodulatorType() == "USB") ||
                    (freqDiff < 0 && demod->getDemodulatorType() == "LSB")) {
                    continue;
                }
                
                if (dist < near_dist) {
                    activeDemodulator = demod;
                    near_dist = dist;
                }
                
                long edge_dist = abs(halfBw - dist);
                if (edge_dist < near_dist) {
                    activeDemodulator = demod;
                    near_dist = edge_dist;
                }
            }
        }
        
        if (activeDemodulator == nullptr) {
            nextDragState = WF_DRAG_NONE;
            SetCursor(wxCURSOR_CROSS);
            return;
        }
        
        wxGetApp().getDemodMgr().setActiveDemodulator(activeDemodulator);
        
        long long freqDiff = activeDemodulator->getFrequency() - freqPos;
        
        if (abs(freqDiff) > (activeDemodulator->getBandwidth() / 3)) {
            
            if (freqDiff > 0) {
                if (activeDemodulator->getDemodulatorType() != "USB") {
                    nextDragState = WF_DRAG_BANDWIDTH_LEFT;
                    SetCursor(wxCURSOR_SIZEWE);
                }
            } else {
                if (activeDemodulator->getDemodulatorType() != "LSB") {
                    nextDragState = WF_DRAG_BANDWIDTH_RIGHT;
                    SetCursor(wxCURSOR_SIZEWE);
                }
            }
            
            mouseTracker.setVertDragLock(true);
            mouseTracker.setHorizDragLock(false);
            setStatusText("Drag to change bandwidth. SPACE or 0-9 for direct frequency input. [, ] to nudge, M for mute, D to delete, C to center, E to edit label, R to record.");
        } else {
            SetCursor(wxCURSOR_SIZING);
            nextDragState = WF_DRAG_FREQUENCY;
            
            mouseTracker.setVertDragLock(true);
            mouseTracker.setHorizDragLock(false);
            setStatusText("Drag to change frequency; SPACE or 0-9 for direct input. [, ] to nudge, M for mute, D to delete, C to center, E to edit label, R to record.");
        }
    }
    else {
        SetCursor(wxCURSOR_CROSS);
        nextDragState = WF_DRAG_NONE;
        if (shiftDown) {
            setStatusText("Click to create a new demodulator or hold ALT to drag new range.");
        }
        else {
            setStatusText(
                "Click to set demodulator frequency or hold ALT to drag range; hold SHIFT to create new. Arrow keys or wheel to navigate/zoom bandwith, C to center. Right-drag or SHIFT+UP/DOWN to adjust visual gain. Shift-R record/stop all.");
        }
    }
}

void WaterfallCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);
    auto demod = wxGetApp().getDemodMgr().getActiveContextModem();

    if (mouseTracker.mouseDown()) {
        if (demod == nullptr) {
            return;
        }
        if (dragState == WF_DRAG_BANDWIDTH_LEFT || dragState == WF_DRAG_BANDWIDTH_RIGHT) {

            int bwDiff = (int) (mouseTracker.getDeltaMouseX() * (float) getBandwidth()) * 2;

            if (dragState == WF_DRAG_BANDWIDTH_LEFT) {
                bwDiff = -bwDiff;
            }

            int currentBW = dragBW;

            currentBW = currentBW + bwDiff;
            if (currentBW > CHANNELIZER_RATE_MAX) {
                currentBW = CHANNELIZER_RATE_MAX;
            }
            if (currentBW < MIN_BANDWIDTH) {
                currentBW = MIN_BANDWIDTH;
            }

            demod->setBandwidth(currentBW);
            dragBW = currentBW;
        }

        if (dragState == WF_DRAG_FREQUENCY) {
            long long bwTarget = getFrequencyAt(mouseTracker.getMouseX()) - dragOfs;
            long long currentFreq = demod->getFrequency();
            long long bwDiff = bwTarget - currentFreq;
            int snap = wxGetApp().getFrequencySnap();

            if (snap > 1) {
                bwDiff = roundf((float)bwDiff/(float)snap)*snap;
            }

            if (bwDiff) {
                demod->setFrequency(currentFreq + bwDiff);
                if (demod->isDeltaLock()) {
                    demod->setDeltaLockOfs(demod->getFrequency() - wxGetApp().getFrequency());
                }
                currentFreq = demod->getFrequency();
                demod->updateLabel(currentFreq);
            }
        }
    } else if (mouseTracker.mouseRightDown() && spectrumCanvas) {
       
        //Right-drag has the same effect on both Waterfall and Spectrum.
        spectrumCanvas->updateScaleFactorFromYMove(mouseTracker.getDeltaMouseY());

    } else {
        updateHoverState();
    }
}

void WaterfallCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);

    updateHoverState();
    dragState = nextDragState;
    wxGetApp().getDemodMgr().updateLastState();

    if (dragState && dragState != WF_DRAG_RANGE) {
        auto demod = wxGetApp().getDemodMgr().getActiveContextModem();
        if (demod) {
            dragOfs = (long long) (mouseTracker.getMouseX() * (float) getBandwidth()) + getCenterFrequency() - (getBandwidth() / 2) - demod->getFrequency();
            dragBW = demod->getBandwidth();
        }
        wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getActiveContextModem(), false);
    }
}

void WaterfallCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
    float movement = (float)event.GetWheelRotation() / (float)event.GetLinesPerAction();

    mouseZoom = 1.0f - movement/1000.0f;
}

void WaterfallCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
    wxGetApp().getDemodMgr().updateLastState();

    bool isNew = shiftDown || (wxGetApp().getDemodMgr().getCurrentModem() == nullptr)
            || (wxGetApp().getDemodMgr().getCurrentModem() && !wxGetApp().getDemodMgr().getCurrentModem()->isActive());

    mouseTracker.setVertDragLock(false);
    mouseTracker.setHorizDragLock(false);

    DemodulatorInstancePtr demod = isNew?nullptr: wxGetApp().getDemodMgr().getCurrentModem();
    DemodulatorInstancePtr activeDemod = isNew?nullptr: wxGetApp().getDemodMgr().getActiveContextModem();

    DemodulatorMgr *mgr = &wxGetApp().getDemodMgr();

    if (mouseTracker.getOriginDeltaMouseX() == 0 && mouseTracker.getOriginDeltaMouseY() == 0) {
        float pos = mouseTracker.getMouseX();
        long long input_center_freq = getCenterFrequency();
        long long freqTarget = input_center_freq - (long long) (0.5 * (float) getBandwidth()) + (long long) ((float) pos * (float) getBandwidth());
        long long demodFreq = demod?demod->getFrequency():freqTarget;
        long long bwDiff = freqTarget - demodFreq;
        long long freq = demodFreq;

         int snap = wxGetApp().getFrequencySnap();

         if (snap > 1) {
             if (demod) {
                 bwDiff = roundf((double)bwDiff/(double)snap)*snap;
                 freq += bwDiff;
             } else {
                 freq = roundl((long double)freq/(double)snap)*snap;
             }
         } else {
             freq += bwDiff;
         }


        if (dragState == WF_DRAG_NONE) {
            if (!isNew && wxGetApp().getDemodMgr().getDemodulators().size()) {
                mgr->updateLastState();
                demod = wxGetApp().getDemodMgr().getCurrentModem();
            } else {
                isNew = true;
                demod = wxGetApp().getDemodMgr().newThread();
                demod->setFrequency(freq);

                demod->setDemodulatorType(mgr->getLastDemodulatorType());
                demod->setBandwidth(mgr->getLastBandwidth());
                demod->setSquelchLevel(mgr->getLastSquelchLevel());
                demod->setSquelchEnabled(mgr->isLastSquelchEnabled());
                demod->setGain(mgr->getLastGain());
                demod->setMuted(mgr->isLastMuted());
                if (mgr->getLastDeltaLock()) {
                    demod->setDeltaLock(true);
                    demod->setDeltaLockOfs(wxGetApp().getFrequency()-freq);
                } else {
                    demod->setDeltaLock(false);
                }
                demod->writeModemSettings(mgr->getLastModemSettings(mgr->getLastDemodulatorType()));
                demod->run();

                wxGetApp().notifyDemodulatorsChanged();
                DemodulatorThread::releaseSquelchLock(nullptr);
            }

            if (!demod) {
                dragState = WF_DRAG_NONE;
                return;
            }

            demod->updateLabel(freq);
            demod->setFrequency(freq);
            if (demod->isDeltaLock()) {
                demod->setDeltaLockOfs(demod->getFrequency() - wxGetApp().getFrequency());
            }
  
            if (isNew) {
                setStatusText("New demodulator at frequency: %s", freq);
            } else {
                setStatusText("Moved demodulator to frequency: %s", freq);
            }

            wxGetApp().getDemodMgr().setActiveDemodulator(demod, false);
          SetCursor(wxCURSOR_SIZING);
            nextDragState = WF_DRAG_FREQUENCY;
            mouseTracker.setVertDragLock(true);
            mouseTracker.setHorizDragLock(false);
        } else {
            if (activeDemod) {
                wxGetApp().getDemodMgr().setActiveDemodulator(activeDemod, false);
                mgr->updateLastState();
                activeDemod->setTracking(true);
                nextDragState = WF_DRAG_FREQUENCY;
            } else {
                nextDragState = WF_DRAG_NONE;
            }
        }
    } else if (dragState == WF_DRAG_RANGE) {
        float width = mouseTracker.getOriginDeltaMouseX();

        float pos;
        std::string last_type = mgr->getLastDemodulatorType();

        if (last_type == "LSB" || last_type == "USB") {
            float pos1 = mouseTracker.getOriginMouseX();
            float pos2 = mouseTracker.getMouseX();

            if (pos2 < pos1) {
                float tmp = pos1;
                pos1 = pos2;
                pos2 = tmp;
            }

            pos = (last_type == "LSB")?pos2:pos1;
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

        int snap = wxGetApp().getFrequencySnap();

        if (snap > 1) {
            freq = roundl((long double)freq/(double)snap)*snap;
        }


        if (!isNew && wxGetApp().getDemodMgr().getDemodulators().size()) {
            mgr->updateLastState();
            demod = wxGetApp().getDemodMgr().getCurrentModem();
        } else {
            demod = wxGetApp().getDemodMgr().newThread();
            demod->setFrequency(freq);
            demod->setDemodulatorType(mgr->getLastDemodulatorType());
            demod->setBandwidth(bw);
            demod->setSquelchLevel(mgr->getLastSquelchLevel());
            demod->setSquelchEnabled(mgr->isLastSquelchEnabled());
            demod->setGain(mgr->getLastGain());
            demod->setMuted(mgr->isLastMuted());
            if (mgr->getLastDeltaLock()) {
                demod->setDeltaLock(true);
                demod->setDeltaLockOfs(wxGetApp().getFrequency()-freq);
            } else {
                demod->setDeltaLock(false);
            }
            demod->writeModemSettings(mgr->getLastModemSettings(mgr->getLastDemodulatorType()));

            demod->run();

            wxGetApp().notifyDemodulatorsChanged();
        }

        if (demod == nullptr) {
            dragState = WF_DRAG_NONE;
            return;
        }

        setStatusText("New demodulator at frequency: %s", freq);

        demod->updateLabel(freq);
        demod->setFrequency(freq);
        demod->setBandwidth(bw);
        mgr->setActiveDemodulator(demod, false);
        mgr->updateLastState();
    }

    dragState = WF_DRAG_NONE;
}

void WaterfallCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
    wxGetApp().getDemodMgr().setActiveDemodulator(nullptr);
    mouseZoom = 1.0;
}

void WaterfallCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
#ifdef _WIN32
	if (wxGetApp().getAppFrame()->canFocus()) {
		this->SetFocus();
	}
#endif
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
}

SpectrumVisualDataQueuePtr WaterfallCanvas::getVisualDataQueue() {
    return visualDataQueue;
}

void WaterfallCanvas::updateCenterFrequency(long long freq) {
    if (isView) {
        setView(freq, getBandwidth());
        if (spectrumCanvas) {
            spectrumCanvas->setView(freq, getBandwidth());
        }
        
        long long minFreq = wxGetApp().getFrequency()-(wxGetApp().getSampleRate()/2);
        long long maxFreq = wxGetApp().getFrequency()+(wxGetApp().getSampleRate()/2);
        
        if (freq - bandwidth / 2 < minFreq) {
            wxGetApp().setFrequency(wxGetApp().getFrequency() - (minFreq - (freq - bandwidth/2)));
        }
        if (freq + bandwidth / 2 > maxFreq) {
            wxGetApp().setFrequency(wxGetApp().getFrequency() + ((freq + bandwidth/2) - maxFreq));
        }
    } else {
        if (spectrumCanvas) {
            spectrumCanvas->setCenterFrequency(freq);
        }
        wxGetApp().setFrequency(freq);
    }

}

void WaterfallCanvas::setLinesPerSecond(int lps) {
    std::lock_guard < std::mutex > lock(tex_update);
    
    linesPerSecond = lps;

    //empty all
    visualDataQueue->flush();
}

void WaterfallCanvas::setMinBandwidth(int min) {
    minBandwidth = min;
}
