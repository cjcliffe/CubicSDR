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
#include "AppFrame.h"
#include <algorithm>

wxBEGIN_EVENT_TABLE(TuningCanvas, wxGLCanvas) EVT_PAINT(TuningCanvas::OnPaint)
EVT_IDLE(TuningCanvas::OnIdle)
EVT_MOTION(TuningCanvas::OnMouseMoved)
EVT_LEFT_DOWN(TuningCanvas::OnMouseDown)
EVT_LEFT_UP(TuningCanvas::OnMouseReleased)
EVT_LEAVE_WINDOW(TuningCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(TuningCanvas::OnMouseEnterWindow)
wxEND_EVENT_TABLE()

TuningCanvas::TuningCanvas(wxWindow *parent, int *attribList) :
        InteractiveCanvas(parent, attribList), dragAccum(0) {

    glContext = new TuningContext(this, &wxGetApp().GetContext(this));

    hoverIndex = 0;
    hoverState = TUNING_HOVER_NONE;

    freqDP = -1.0;
    freqW = (1.0 / 3.0) * 2.0;

    bwDP = -1.0 + (2.25 / 3.0);
    bwW = (1.0 / 4.0) * 2.0;

    centerDP = -1.0 + (2.0 / 3.0) * 2.0;
    centerW = (1.0 / 3.0) * 2.0;
}

TuningCanvas::~TuningCanvas() {

}

void TuningCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
#ifdef __APPLE__    // force half-rate?
    glFinish();
#endif
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    initGLExtensions();
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();

    DemodulatorInstance *activeDemod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    long long freq = 0;
    if (activeDemod != NULL) {
        freq = activeDemod->getFrequency();
    }
    long long bw = wxGetApp().getDemodMgr().getLastBandwidth();
    long long center = wxGetApp().getFrequency();

    if (mouseTracker.mouseDown()) {
        glContext->Draw(ThemeMgr::mgr.currentTheme->tuningBar.r, ThemeMgr::mgr.currentTheme->tuningBar.g, ThemeMgr::mgr.currentTheme->tuningBar.b,
                0.6, mouseTracker.getOriginMouseX(), mouseTracker.getMouseX());
    }

    bool top = mouseTracker.getMouseY() >= 0.5;
    bool bottom = mouseTracker.getMouseY() <= 0.5;

    RGBColor clr = top ? ThemeMgr::mgr.currentTheme->tuningBarUp : ThemeMgr::mgr.currentTheme->tuningBarDown;

    RGBColor clrDark = ThemeMgr::mgr.currentTheme->tuningBar;
    RGBColor clrMid = ThemeMgr::mgr.currentTheme->tuningBar;
    RGBColor clrLight = ThemeMgr::mgr.currentTheme->tuningBar;

    clrDark.r*=0.5;clrDark.g*=0.5;clrDark.b*=0.5;
    clrLight.r*=2.0;clrLight.g*=2.0;clrLight.b*=2.0;

    glContext->DrawTunerBarIndexed(1, 3, 11, freqDP, freqW, clrDark, 0.25, true, true); // freq
    glContext->DrawTunerBarIndexed(4, 6, 11, freqDP, freqW, clrMid, 0.25, true, true);
    glContext->DrawTunerBarIndexed(7, 9, 11, freqDP, freqW, clrDark, 0.25, true, true);
    glContext->DrawTunerBarIndexed(10, 11, 11, freqDP, freqW, clrMid, 0.25, true, true);

    glContext->DrawTunerBarIndexed(1, 3, 7, bwDP, bwW, clrDark, 0.25, true, true); // bw
    glContext->DrawTunerBarIndexed(4, 6, 7, bwDP, bwW, clrMid, 0.25, true, true);
    glContext->DrawTunerBarIndexed(7, 7, 7, bwDP, bwW, clrDark, 0.25, true, true);

    glContext->DrawTunerBarIndexed(1, 3, 11, centerDP, centerW, clrDark, 0.25, true, true); // freq
      glContext->DrawTunerBarIndexed(4, 6, 11, centerDP, centerW, clrMid, 0.25, true, true);
      glContext->DrawTunerBarIndexed(7, 9, 11, centerDP, centerW, clrDark, 0.25, true, true);
      glContext->DrawTunerBarIndexed(10, 11, 11, centerDP, centerW, clrMid, 0.25, true, true);

    if (hoverIndex > 0) {
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
        }
    }

    glContext->DrawTuner(freq, 11, freqDP, freqW);
    glContext->DrawTuner(bw, 7, bwDP, bwW);
    glContext->DrawTuner(center, 11, centerDP, centerW);

    glContext->DrawEnd();

    SwapBuffers();
}

void TuningCanvas::OnIdle(wxIdleEvent &event) {
    if (mouseTracker.mouseDown()) {
        DemodulatorInstance *activeDemod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

        float dragDelta = mouseTracker.getMouseX() - mouseTracker.getOriginMouseX();

        dragAccum += dragDelta;

        float moveVal = dragAccum * 10.0;

        if (uxDown > 0.275) {
            wxGetApp().setFrequency(
                    wxGetApp().getFrequency()
                            + (int) (dragAccum * fabs(dragAccum * 10.0) * fabs(dragAccum * 10.0) * (float) wxGetApp().getSampleRate()));
        } else if (fabs(moveVal) >= 1.0) {
            if (uxDown < -0.275) {
                if (activeDemod != NULL) {
                    long long freq = activeDemod->getFrequency() + (int) (moveVal * fabs(moveVal) * fabs(moveVal) * fabs(moveVal));
                    activeDemod->setFrequency(freq);
                    activeDemod->updateLabel(freq);
                }
            } else {
                int amt = (int) (moveVal * fabs(moveVal) * fabs(moveVal) * fabs(moveVal));
                if (activeDemod != NULL) {
                    activeDemod->setBandwidth(activeDemod->getBandwidth() + amt);
                } else {
                    wxGetApp().getDemodMgr().setLastBandwidth(wxGetApp().getDemodMgr().getLastBandwidth() + amt);
                }
            }
        }

        while (fabs(dragAccum * 10.0) >= 1.0) {
            if (dragAccum > 0) {
                dragAccum -= 1.0 / 10.0;
            } else {
                dragAccum += 1.0 / 10.0;
            }
        }
    }

    Refresh(false);
}

void TuningCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);

    int index = 0;

    index = glContext->GetTunerDigitIndex(mouseTracker.getMouseX(), 11, freqDP, freqW); // freq
    if (index > 0) {
        hoverIndex = index;
        hoverState = TUNING_HOVER_FREQ;
        return;
    }

    index = glContext->GetTunerDigitIndex(mouseTracker.getMouseX(), 7, bwDP, bwW); // bw
    if (index > 0) {
        hoverIndex = index;
        hoverState = TUNING_HOVER_BW;
        return;
    }

    index = glContext->GetTunerDigitIndex(mouseTracker.getMouseX(), 11, centerDP, centerW); // center
    if (index > 0) {
        hoverIndex = index;
        hoverState = TUNING_HOVER_CENTER;
        return;
    }

    hoverIndex = 0;
    hoverState = TUNING_HOVER_NONE;
}

void TuningCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
    mouseTracker.setVertDragLock(true);

    uxDown = 2.0 * (mouseTracker.getMouseX() - 0.5);

    dragAccum = 0;
    SetCursor(wxCURSOR_IBEAM);
}

void TuningCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
}

void TuningCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
    mouseTracker.setVertDragLock(false);
    SetCursor(wxCURSOR_ARROW);
}

void TuningCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
    hoverIndex = 0;
    hoverState = TUNING_HOVER_NONE;
}

void TuningCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_ARROW);
    hoverIndex = 0;
    hoverState = TUNING_HOVER_NONE;
}

void TuningCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}
