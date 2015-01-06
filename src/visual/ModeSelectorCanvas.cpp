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
InteractiveCanvas(parent, attribList), currentSelection(-1), numChoices(0) {

    glContext = new ModeSelectorContext(this, &wxGetApp().GetContext(this));
}

ModeSelectorCanvas::~ModeSelectorCanvas() {

}

int ModeSelectorCanvas::getHoveredSelection() {
    if (!mouseTracker.mouseInView()) {
        return -1;
    }

    float ypos = 1.0 - (mouseTracker.getMouseY() * 2.0);
    float yval = (int) (((ypos + 1.0) / 2.0) * (float) numChoices);

    return yval;
}

void ModeSelectorCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->DrawBegin();

    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    int demodType = 0;
    if (demod) {
        demodType = demod->getDemodulatorType();
    }

    int yval = getHoveredSelection();

    for (int i = 0; i < numChoices; i++) {
        glContext->DrawSelector(selections[i].label, i, numChoices, i == currentSelection || yval == i, (yval == i)?0.9:0.75, (yval == i)?0.9:0.75, (yval == i)?0.2:0.75, 1.0);
    }

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

    const wxSize ClientSize = GetClientSize();

    if (mouseTracker.getOriginDeltaMouseX() < 2.0 / ClientSize.y) {
        currentSelection = getHoveredSelection();
    }

    SetCursor (wxCURSOR_ARROW);
}

void ModeSelectorCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor (wxCURSOR_CROSS);
}

void ModeSelectorCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
    SetCursor (wxCURSOR_ARROW);
}

void ModeSelectorCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

void ModeSelectorCanvas::setNumChoices(int numChoices_in) {
    numChoices = numChoices_in;
}

void ModeSelectorCanvas::addChoice(int value, std::string label) {
    selections.push_back(ModeSelectorMode(value, label));
    numChoices = selections.size();
}

void ModeSelectorCanvas::setSelection(int value) {
    for (int i = 0; i < numChoices; i++) {
        if (selections[i].value == value) {
            currentSelection = i;
            return;
        }
    }
    currentSelection = -1;
}

int ModeSelectorCanvas::getSelection() {
    if (currentSelection == -1) {
        return -1;
    }
    return selections[currentSelection].value;
}


