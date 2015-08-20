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
#include <cmath>


wxBEGIN_EVENT_TABLE(ScopeCanvas, wxGLCanvas) EVT_PAINT(ScopeCanvas::OnPaint)
EVT_IDLE(ScopeCanvas::OnIdle)
EVT_MOTION(ScopeCanvas::OnMouseMoved)
EVT_LEFT_DOWN(ScopeCanvas::OnMouseDown)
EVT_LEFT_UP(ScopeCanvas::OnMouseReleased)
EVT_RIGHT_DOWN(ScopeCanvas::OnMouseRightDown)
EVT_RIGHT_UP(ScopeCanvas::OnMouseRightReleased)
EVT_LEAVE_WINDOW(ScopeCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(ScopeCanvas::OnMouseEnterWindow)
wxEND_EVENT_TABLE()

ScopeCanvas::ScopeCanvas(wxWindow *parent, int *attribList) : InteractiveCanvas(parent, attribList), stereo(false), ppmMode(false), ctr(0), ctrTarget(0), dragAccel(0) {

    glContext = new ScopeContext(this, &wxGetApp().GetContext(this));
    inputData.set_max_num_items(1);
    bgPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_Y);
    bgPanel.setSize(1.0, 0.5);
    bgPanel.setPosition(0.0, -0.5);
    panelSpacing = 0.2;
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

        if (avData) {
            if (avData->waveform_points.size()) {
                scopePanel.setPoints(avData->waveform_points);
                setStereo(avData->channels == 2);
            }
            
            avData->decRefCount();
        }
    }

    glContext->SetCurrent(*this);
    initGLExtensions();

    glViewport(0, 0, ClientSize.x, ClientSize.y);
    
    glContext->DrawBegin();
    
    bgPanel.setFillColor(ThemeMgr::mgr.currentTheme->scopeBackground * 3.0, RGBA4f(0,0,0,0));
    bgPanel.calcTransform(CubicVR::mat4::identity());
    bgPanel.draw();

    scopePanel.setMode(stereo?ScopePanel::SCOPE_MODE_2Y:ScopePanel::SCOPE_MODE_Y);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glLoadMatrixf(CubicVR::mat4::perspective(45.0, 1.0, 1.0, 1000.0));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    CubicVR::mat4 modelView = CubicVR::mat4::lookat(0, 0, -1.2, 0, 0, 0, 0, -1, 0);

    float panelWidth = 1.0;
    float panelInterval = (panelWidth * 2.0 + panelSpacing);

    if (!mouseTracker.mouseDown()) {
        if (!dragAccel) {
            ctrTarget = round(ctr / panelInterval);
            if (ctrTarget < -1.0) {
                ctrTarget = -1.0;
            } else if (ctrTarget > 0.0) {
                ctrTarget = 0.0;
            }
            ctrTarget *= panelInterval;
            if (ctr != ctrTarget) {
                ctr += (ctrTarget-ctr)*0.2;
            }
        } else {
            dragAccel -= dragAccel * 0.01;
            if (abs(dragAccel) < 0.1 || ctr < ctrTarget-panelInterval/2.0 || ctr > ctrTarget+panelInterval/2.0 ) {
                dragAccel = 0;
            } else {
                ctr += dragAccel;
            }
        }
    }
    
    scopePanel.setPosition(ctr, 0);
    float roty = atan2(scopePanel.pos[0],1.2);
    scopePanel.rot[1] = -(roty * (180.0 / M_PI));
    scopePanel.calcTransform(modelView);
    scopePanel.draw();

    scopePanel.setPosition(panelInterval+ctr, 0);
    roty = atan2(scopePanel.pos[0],1.2);
    scopePanel.rot[1] = -(roty * (180.0 / M_PI));
    scopePanel.calcTransform(modelView);
    scopePanel.draw();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glContext->DrawTunerTitles(ppmMode);
    if (!deviceName.empty()) {
        glContext->DrawDeviceName(deviceName);
    }
    glContext->DrawEnd();

    SwapBuffers();
}


void ScopeCanvas::OnIdle(wxIdleEvent &event) {
    Refresh();
    event.RequestMore();
}

ScopeRenderDataQueue *ScopeCanvas::getInputQueue() {
    return &inputData;
}

void ScopeCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);
    if (mouseTracker.mouseDown()) {
        dragAccel = 4.0*mouseTracker.getDeltaMouseX();
        ctr += dragAccel;
    }
}

void ScopeCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    
}

void ScopeCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
    
}

void ScopeCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
    
}

void ScopeCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseEnterWindow(event);
    
}

void ScopeCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    
}

