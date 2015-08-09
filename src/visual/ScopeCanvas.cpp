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
EVT_IDLE(ScopeCanvas::OnIdle)
wxEND_EVENT_TABLE()

ScopeCanvas::ScopeCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), stereo(false), ppmMode(false) {

    glContext = new ScopeContext(this, &wxGetApp().GetContext(this));
    inputData.set_max_num_items(1);
}

ScopeCanvas::~ScopeCanvas() {

}

void ScopeCanvas::setStereo(bool state) {
    stereo = state;
}

void ScopeCanvas::setDeviceName(std::string device_name) {
    deviceName = device_name;
    deviceName.append(" ");
}

void ScopeCanvas::setPPMMode(bool ppmMode) {
    this->ppmMode = ppmMode;
}

bool ScopeCanvas::getPPMMode() {
    return ppmMode;
}

void ScopeCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    if (!inputData.empty()) {
        ScopeRenderData *avData;
        inputData.pop(avData);

        if (!avData) {
            return;
        }
        
        int iMax = avData->waveform_points.size();
        
        if (!iMax) {
            avData->decRefCount();
            return;
        }
  
        scopePanel.setPoints(avData->waveform_points);
        setStereo(avData->channels == 2);
        
        avData->decRefCount();
    }

    glContext->SetCurrent(*this);
    initGLExtensions();

    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();
    scopePanel.setMode(stereo?ScopePanel::SCOPE_MODE_2Y:ScopePanel::SCOPE_MODE_Y);
    scopePanel.calcTransform(CubicVR::mat4::identity());
    scopePanel.draw();
    glContext->DrawTunerTitles(ppmMode);
    if (!deviceName.empty()) {
        glContext->DrawDeviceName(deviceName);
    }
    glContext->DrawEnd();


    SwapBuffers();
}

void ScopeCanvas::OnIdle(wxIdleEvent &event) {
    event.Skip();
}

ScopeRenderDataQueue *ScopeCanvas::getInputQueue() {
    return &inputData;
}
