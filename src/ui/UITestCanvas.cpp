// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "UITestCanvas.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include <algorithm>

wxBEGIN_EVENT_TABLE(UITestCanvas, wxGLCanvas) EVT_PAINT(UITestCanvas::OnPaint)
EVT_IDLE(UITestCanvas::OnIdle)
EVT_MOTION(UITestCanvas::OnMouseMoved)
EVT_LEFT_DOWN(UITestCanvas::OnMouseDown)
EVT_LEFT_UP(UITestCanvas::OnMouseReleased)
EVT_LEAVE_WINDOW(UITestCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(UITestCanvas::OnMouseEnterWindow)
wxEND_EVENT_TABLE()

UITestCanvas::UITestCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs) :
InteractiveCanvas(parent, dispAttrs) {
    
    glContext = new UITestContext(this, &wxGetApp().GetContext(this), wxGetApp().GetContextAttributes());
}

UITestCanvas::~UITestCanvas() = default;

void UITestCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
  //  wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();
    
    glContext->SetCurrent(*this);
    initGLExtensions();
    
    glViewport(0, 0, ClientSize.x, ClientSize.y);
    
    glContext->DrawBegin();

    glContext->Draw();
    
    glContext->DrawEnd();
    
    SwapBuffers();
}

void UITestCanvas::OnIdle(wxIdleEvent& /* event */) {
    Refresh(false);
}

void UITestCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);

}

void UITestCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
}

void UITestCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
}

void UITestCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
}

void UITestCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
}

void UITestCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
}
