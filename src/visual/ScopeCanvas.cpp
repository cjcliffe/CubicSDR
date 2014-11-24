#include "ScopeCanvas.h"

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

wxBEGIN_EVENT_TABLE(ScopeCanvas, wxGLCanvas) EVT_PAINT(ScopeCanvas::OnPaint)
EVT_KEY_DOWN(ScopeCanvas::OnKeyDown)
EVT_IDLE(ScopeCanvas::OnIdle)
wxEND_EVENT_TABLE()

ScopeCanvas::ScopeCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), parent(parent), frameTimer(0) {

    glContext = new ScopeContext(this, &wxGetApp().GetContext(this));
    timer.start();
}

ScopeCanvas::~ScopeCanvas() {

}

void ScopeCanvas::setWaveformPoints(std::vector<float> &waveform_points_in) {
    waveform_points = waveform_points_in;
}

void ScopeCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->Plot(waveform_points);

    SwapBuffers();
}

void ScopeCanvas::OnKeyDown(wxKeyEvent& event) {
    float angle = 5.0;

    unsigned int freq;
    switch (event.GetKeyCode()) {
    case WXK_RIGHT:
        freq = wxGetApp().getFrequency();
        freq += 100000;
        wxGetApp().setFrequency(freq);
        break;
    case WXK_LEFT:
        freq = wxGetApp().getFrequency();
        freq -= 100000;
        wxGetApp().setFrequency(freq);
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

void ScopeCanvas::OnIdle(wxIdleEvent &event) {
//    timer.update();
//    frameTimer += timer.lastUpdateSeconds();
//    if (frameTimer > 1.0/30.0) {
        Refresh(false);
//        frameTimer = 0;
//    }
}
