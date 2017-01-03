// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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

ScopeCanvas::ScopeCanvas(wxWindow *parent, int *dispAttrs) : InteractiveCanvas(parent, dispAttrs), ppmMode(false), ctr(0), ctrTarget(0), dragAccel(0), helpTip("") {

    glContext = new ScopeContext(this, &wxGetApp().GetContext(this));
    inputData.set_max_num_items(2);
    bgPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_Y);
    bgPanel.setSize(1.0, 0.5f);
    bgPanel.setPosition(0.0, -0.5f);
    panelSpacing = 0.4f;
    
    parentPanel.addChild(&scopePanel);
    parentPanel.addChild(&spectrumPanel);
    parentPanel.setFill(GLPanel::GLPANEL_FILL_NONE);
    scopePanel.setSize(1.0,-1.0);
    spectrumPanel.setSize(1.0,-1.0);
    spectrumPanel.setShowDb(true);
}

ScopeCanvas::~ScopeCanvas() {

}

bool ScopeCanvas::scopeVisible() {
    float panelInterval = (2.0 + panelSpacing);
    
    ctrTarget = abs(round(ctr / panelInterval));

    if (ctrTarget == 0 || dragAccel || (ctr != ctrTarget)) {
        return true;
    }
    
    return false;
}

bool ScopeCanvas::spectrumVisible() {
    float panelInterval = (2.0 + panelSpacing);
    
    ctrTarget = abs(round(ctr / panelInterval));
 
    if (ctrTarget == 1 || dragAccel || (ctr != ctrTarget)) {
        return true;
    }
    
    return false;
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

void ScopeCanvas::setShowDb(bool showDb) {
    this->showDb = showDb;
}

bool ScopeCanvas::getShowDb() {
    return showDb;
}

void ScopeCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();
    
    ScopeRenderData *avData;
    while (inputData.try_pop(avData)) {
       
        
        if (!avData->spectrum) {
            scopePanel.setMode(avData->mode);
            if (avData->waveform_points.size()) {
                scopePanel.setPoints(avData->waveform_points);
            }
            avData->decRefCount();
        } else {
            if (avData->waveform_points.size()) {
                spectrumPanel.setPoints(avData->waveform_points);
                spectrumPanel.setFloorValue(avData->fft_floor);
                spectrumPanel.setCeilValue(avData->fft_ceil);
                spectrumPanel.setBandwidth((avData->sampleRate/2)*1000);
                spectrumPanel.setFreq((avData->sampleRate/4)*1000);
                spectrumPanel.setFFTSize(avData->fft_size);
                spectrumPanel.setShowDb(showDb);
            }
            
            avData->decRefCount();
        }
    }

    glContext->SetCurrent(*this);
    initGLExtensions();

    glViewport(0, 0, ClientSize.x, ClientSize.y);
    
    // TODO: find out why frontbuffer drawing has stopped working in wx 3.1.0?
//    if (scopePanel.getMode() == ScopePanel::SCOPE_MODE_XY && !spectrumVisible()) {
//        glDrawBuffer(GL_FRONT);
//        glContext->DrawBegin(false);
//    } else {
//        glDrawBuffer(GL_BACK);
        glContext->DrawBegin();

        bgPanel.setFillColor(ThemeMgr::mgr.currentTheme->scopeBackground * 3.0, RGBA4f(0,0,0,1));
        bgPanel.calcTransform(CubicVR::mat4::identity());
        bgPanel.draw();
//    }
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glLoadMatrixf(CubicVR::mat4::perspective(45.0, 1.0, 1.0, 1000.0));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    CubicVR::mat4 modelView = CubicVR::mat4::lookat(0, 0, -1.205f, 0, 0, 0, 0, -1, 0);

    float panelWidth = 1.0;
    float panelInterval = (panelWidth * 2.0 + panelSpacing);

    if (!mouseTracker.mouseDown()) {
        ctrTarget = round(ctr / panelInterval);
        if (ctrTarget < -1.0) {
            ctrTarget = -1.0;
        } else if (ctrTarget > 0.0) {
            ctrTarget = 0.0;
        }
        ctrTarget *= panelInterval;
        if (!dragAccel) {
            if (ctr != ctrTarget) {
                ctr += (ctrTarget-ctr)*0.2;
            }
            if (abs(ctr - ctrTarget) < 0.001) {
                ctr=ctrTarget;
            }
        } else {
            dragAccel -= dragAccel * 0.1;
            if ((abs(dragAccel) < 0.2) || (ctr < (ctrTarget-panelInterval/2.0)) || (ctr > (ctrTarget+panelInterval/2.0)) ) {
                dragAccel = 0;
            } else {
                ctr += dragAccel;
            }
        }
    }

    float roty = 0;
    
    scopePanel.setPosition(ctr, 0);
    if (scopeVisible()) {
        scopePanel.contentsVisible = true;
        roty = atan2(scopePanel.pos[0],1.2);
        scopePanel.rot[1] = -(roty * (180.0 / M_PI));
    } else {
        scopePanel.contentsVisible = false;
    }
    
    spectrumPanel.setPosition(panelInterval+ctr, 0);
    if (spectrumVisible()) {
        spectrumPanel.setFillColor(ThemeMgr::mgr.currentTheme->scopeBackground * 2.0, RGBA4f(0,0,0,1));
        spectrumPanel.contentsVisible = true;
        roty = atan2(spectrumPanel.pos[0],1.2);
        spectrumPanel.rot[1] = -(roty * (180.0 / M_PI));
    } else {
        spectrumPanel.contentsVisible = false;
    }

    parentPanel.calcTransform(modelView);
    parentPanel.draw();

    if (spectrumVisible()) {
        spectrumPanel.drawChildren();
    }
    
    glLoadMatrixf(scopePanel.transform);
    if (!deviceName.empty()) {
        glContext->DrawDeviceName(deviceName);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glContext->DrawTunerTitles(ppmMode);
    glContext->DrawEnd();

//    if (scopePanel.getMode() != ScopePanel::SCOPE_MODE_XY || spectrumVisible()) {
        SwapBuffers();
//    }
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

void ScopeCanvas::OnMouseWheelMoved(wxMouseEvent& /* event */) {
    
}

void ScopeCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
    
}

void ScopeCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
    
}

void ScopeCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseEnterWindow(event);
    if (!helpTip.empty()) {
        setStatusText(helpTip);
    }
    SetCursor(wxCURSOR_SIZEWE);
}

void ScopeCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    
}


void ScopeCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

