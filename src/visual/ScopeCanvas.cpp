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
        wxFULL_REPAINT_ON_RESIZE), parent(parent), stereo(false) {

    glContext = new ScopeContext(this, &wxGetApp().GetContext(this));
}

ScopeCanvas::~ScopeCanvas() {

}

void ScopeCanvas::setWaveformPoints(std::vector<float> &waveform_points_in) {
    waveform_points = waveform_points_in;
}

void ScopeCanvas::setStereo(bool state) {
    stereo = state;
}

void ScopeCanvas::setDeviceName(std::string device_name) {
    deviceName = device_name;
    deviceName.append(" ");
}

void ScopeCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    if (!wxGetApp().getAudioVisualQueue()->empty()) {
        AudioThreadInput *demodAudioData;
        wxGetApp().getAudioVisualQueue()->pop(demodAudioData);
        if (demodAudioData && demodAudioData->data.size()) {
            if (waveform_points.size() != demodAudioData->data.size() * 2) {
                waveform_points.resize(demodAudioData->data.size() * 2);
            }

            for (int i = 0, iMax = demodAudioData->data.size(); i < iMax; i++) {
                waveform_points[i * 2 + 1] = demodAudioData->data[i] * 0.5f;
                waveform_points[i * 2] = ((double) i / (double) iMax);
            }

            setStereo(demodAudioData->channels == 2);
        } else {
            std::cout << "Incoming Demodulator data empty?" << std::endl;
        }
    }

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();
    if (!deviceName.empty()) {
        glContext->DrawDeviceName(deviceName);
    }
    glContext->Plot(waveform_points, stereo);
    glContext->DrawEnd();


    SwapBuffers();
}

void ScopeCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}
