#include "ModeSelectorCanvas.h"

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

wxBEGIN_EVENT_TABLE(ModeSelectorCanvas, wxGLCanvas) EVT_PAINT(ModeSelectorCanvas::OnPaint)
EVT_IDLE(ModeSelectorCanvas::OnIdle)
EVT_MOTION(ModeSelectorCanvas::OnMouseMoved)
EVT_LEFT_DOWN(ModeSelectorCanvas::OnMouseDown)
EVT_LEFT_UP(ModeSelectorCanvas::OnMouseReleased)
EVT_LEAVE_WINDOW(ModeSelectorCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(ModeSelectorCanvas::OnMouseEnterWindow)
wxEND_EVENT_TABLE()

ModeSelectorCanvas::ModeSelectorCanvas(wxWindow *parent, int *attribList) :
        InteractiveCanvas(parent, attribList) {

    glContext = new ModeSelectorContext(this, &wxGetApp().GetContext(this));
}

ModeSelectorCanvas::~ModeSelectorCanvas() {

}

void ModeSelectorCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();

    glContext->DrawSelector("FM", 1, 4, true, 0.75, 0.75, 0.75, 1.0);
    glContext->DrawSelector("AM", 2, 4, true, 0.75, 0.75, 0.75, 1.0);
    glContext->DrawSelector("LSB", 3, 4, true, 0.75, 0.75, 0.75, 1.0);
    glContext->DrawSelector("USB", 4, 4, true, 0.75, 0.75, 0.75, 1.0);

    glContext->DrawEnd();

    SwapBuffers();
}

void ModeSelectorCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

void ModeSelectorCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);
}

void ModeSelectorCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
    mouseTracker.setHorizDragLock(true);
    mouseTracker.setVertDragLock(true);
}

void ModeSelectorCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
}

void ModeSelectorCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
    mouseTracker.setHorizDragLock(false);
    mouseTracker.setVertDragLock(false);
    SetCursor(wxCURSOR_ARROW);
}

void ModeSelectorCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
}

void ModeSelectorCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_ARROW);
}

void ModeSelectorCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}
