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
}

TuningCanvas::~TuningCanvas() {

}

void TuningCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();

    DemodulatorInstance *activeDemod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    if (activeDemod != NULL) {
        glContext->DrawDemodFreqBw(activeDemod->getFrequency(), activeDemod->getBandwidth(), wxGetApp().getFrequency());
    } else {
        glContext->DrawDemodFreqBw(0, wxGetApp().getDemodMgr().getLastBandwidth(), wxGetApp().getFrequency());
    }

    if (mouseTracker.mouseDown()) {
        glContext->Draw(ThemeMgr::mgr.currentTheme->tuningBar.r, ThemeMgr::mgr.currentTheme->tuningBar.g, ThemeMgr::mgr.currentTheme->tuningBar.b,
                0.6, mouseTracker.getOriginMouseX(), mouseTracker.getMouseX());
    }

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
                    activeDemod->setFrequency(activeDemod->getFrequency() + (int) (moveVal * fabs(moveVal) * fabs(moveVal) * fabs(moveVal)));
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
    SetCursor(wxCURSOR_SIZEWE);
}

void TuningCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
}

void TuningCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_SIZEWE);
}

void TuningCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}
