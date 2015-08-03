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
EVT_LEAVE_WINDOW(SpectrumCanvas::OnMouseLeftWindow)
EVT_MOUSEWHEEL(SpectrumCanvas::OnMouseWheelMoved)
wxEND_EVENT_TABLE()

SpectrumCanvas::SpectrumCanvas(wxWindow *parent, int *attribList) :
        InteractiveCanvas(parent, attribList), waterfallCanvas(NULL) {

    glContext = new SpectrumContext(this, &wxGetApp().GetContext(this));

    mouseTracker.setVertDragLock(true);

    SetCursor(wxCURSOR_SIZEWE);
}

SpectrumCanvas::~SpectrumCanvas() {

}

void SpectrumCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
#ifdef __APPLE__    // force half-rate?
    glFinish();
#endif
    const wxSize ClientSize = GetClientSize();
    
    if (visualDataQueue.empty()) {
        return;
    }
    
    SpectrumVisualData *vData;
    
    visualDataQueue.pop(vData);
    
    if (!vData) {
        return;
    }
    
    spectrum_points.assign(vData->spectrum_points.begin(),vData->spectrum_points.end());
    
    vData->decRefCount();
    
    glContext->SetCurrent(*this);
    initGLExtensions();

    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->BeginDraw(ThemeMgr::mgr.currentTheme->fftBackground.r, ThemeMgr::mgr.currentTheme->fftBackground.g, ThemeMgr::mgr.currentTheme->fftBackground.b);
    glContext->Draw(spectrum_points, getCenterFrequency(), getBandwidth());

    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        glContext->DrawDemodInfo(demods[i], ThemeMgr::mgr.currentTheme->fftHighlight, getCenterFrequency(), getBandwidth());
    }

    glContext->EndDraw();

    SwapBuffers();
}


void SpectrumCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
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
        setStatusText("Set center frequency: %s", freq);
    }
}

void SpectrumCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);
    if (mouseTracker.mouseDown()) {
        int freqChange = mouseTracker.getDeltaMouseX() * getBandwidth();

        if (freqChange != 0) {
            moveCenterFrequency(freqChange);
        }
    } else {
        setStatusText("Click and drag to adjust center frequency.");
    }
}

void SpectrumCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
    SetCursor(wxCURSOR_CROSS);
}

void SpectrumCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
}

void SpectrumCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
    SetCursor(wxCURSOR_SIZEWE);
}

void SpectrumCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_SIZEWE);
}

void SpectrumCanvas::attachWaterfallCanvas(WaterfallCanvas* canvas_in) {
    waterfallCanvas = canvas_in;
}

SpectrumContext* SpectrumCanvas::getSpectrumContext() {
    return glContext;
}

SpectrumVisualDataQueue *SpectrumCanvas::getVisualDataQueue() {
    return &visualDataQueue;
}