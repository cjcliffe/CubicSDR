#pragma once

#include "wx/window.h"

class MouseTracker {
public:
    MouseTracker(wxWindow *target) :
            target(target), mouseX(0), mouseY(0), lastMouseX(0), lastMouseY(0), deltaMouseX(0), deltaMouseY(0), isMouseDown(false), vertDragLock(false) {

    }

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    float getDeltaMouseX();
    float getDeltaMouseY();
    float getLastMouseX();
    float getLastMouseY();
    float getMouseX();
    float getMouseY();

    void setVertDragLock(bool dragLock);
    bool mouseDown();

private:
    float mouseX, mouseY;
    float lastMouseX, lastMouseY;
    float deltaMouseX, deltaMouseY;
    bool isMouseDown;
    bool vertDragLock;
    wxWindow *target;
};
