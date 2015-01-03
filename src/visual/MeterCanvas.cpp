#include "MeterCanvas.h"

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

wxBEGIN_EVENT_TABLE(MeterCanvas, wxGLCanvas) EVT_PAINT(MeterCanvas::OnPaint)
EVT_IDLE(MeterCanvas::OnIdle)
EVT_MOTION(MeterCanvas::OnMouseMoved)
EVT_LEFT_DOWN(MeterCanvas::OnMouseDown)
EVT_LEFT_UP(MeterCanvas::OnMouseReleased)
EVT_LEAVE_WINDOW(MeterCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(MeterCanvas::OnMouseEnterWindow)
wxEND_EVENT_TABLE()

MeterCanvas::MeterCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), parent(parent), level(0), level_max(1), inputValue(0), userInputValue(0), shiftDown(false), altDown(false), ctrlDown(false) {

    glContext = new MeterContext(this, &wxGetApp().GetContext(this));
    mTracker.setTarget(this);
}

MeterCanvas::~MeterCanvas() {

}

void MeterCanvas::setLevel(float level_in) {
    level = level_in;
}
float MeterCanvas::getLevel() {
    return level;
}

void MeterCanvas::setMax(float max_in) {
    level_max = max_in;
}

bool MeterCanvas::setInputValue(float slider_in) {
    userInputValue = inputValue = slider_in;
}
bool MeterCanvas::inputChanged() {
    return (inputValue != userInputValue);
}
float MeterCanvas::getInputValue() {
    inputValue = userInputValue;
    return userInputValue;
}

void MeterCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();
    if (mTracker.mouseInView()) {
        glContext->Draw(0.4, 0.4, 0.4, 0.5, mTracker.getMouseY());
    }

    glContext->Draw(0.1, 0.75, 0.1, 0.5, level/level_max);

    glContext->Draw(0.75, 0.1, 0.1, 0.5, userInputValue/level_max);

    glContext->DrawEnd();

    SwapBuffers();
}

void MeterCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

void MeterCanvas::OnMouseMoved(wxMouseEvent& event) {
    mTracker.OnMouseMoved(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    if (mTracker.mouseDown()) {
        userInputValue = mTracker.getMouseY()*level_max;
    }
}

void MeterCanvas::OnMouseDown(wxMouseEvent& event) {
    mTracker.OnMouseDown(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    userInputValue = mTracker.getMouseY()*level_max;
    mTracker.setHorizDragLock(true);
}

void MeterCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    mTracker.OnMouseWheelMoved(event);
}

void MeterCanvas::OnMouseReleased(wxMouseEvent& event) {
    mTracker.OnMouseReleased(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();

    userInputValue = mTracker.getMouseY()*level_max;
}

void MeterCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    mTracker.OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
}

void MeterCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    mTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
}
