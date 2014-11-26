#include "MouseTracker.h"

void MouseTracker::OnMouseMoved(wxMouseEvent& event) {
    if (isMouseDown) {
        const wxSize ClientSize = target->GetClientSize();
        float mouseX = (float) event.m_x / (float) ClientSize.x;
        float mouseY = (float) event.m_y / (float) ClientSize.y;

        deltaMouseX = mouseX - lastMouseX;
        deltaMouseY = mouseY - lastMouseY;

        lastMouseX = mouseX;

        if (vertDragLock && mouseY != lastMouseY) {
            target->WarpPointer(event.m_x, lastMouseY * ClientSize.y);
        } else {
            lastMouseY = mouseY;
        }
    }
}

void MouseTracker::OnMouseDown(wxMouseEvent& event) {
    const wxSize ClientSize = target->GetClientSize();

    lastMouseX = (float) event.m_x / (float) ClientSize.x;
    lastMouseY = (float) event.m_y / (float) ClientSize.y;

    isMouseDown = true;
}

void MouseTracker::OnMouseWheelMoved(wxMouseEvent& event) {
//    std::cout << "wheel?" << std::endl;
}

void MouseTracker::OnMouseReleased(wxMouseEvent& event) {
    isMouseDown = false;
}

void MouseTracker::OnMouseLeftWindow(wxMouseEvent& event) {
    isMouseDown = false;
}

float MouseTracker::getDeltaMouseX() {
    return deltaMouseX;
}

float MouseTracker::getDeltaMouseY() {
    return deltaMouseY;
}

float MouseTracker::getLastMouseX() {
    return lastMouseX;
}

float MouseTracker::getLastMouseY() {
    return lastMouseY;
}

float MouseTracker::getMouseX() {
    return mouseX;
}

float MouseTracker::getMouseY() {
    return mouseY;
}

void MouseTracker::setVertDragLock(bool dragLock) {
    vertDragLock = dragLock;
}

bool MouseTracker::mouseDown() {
    return isMouseDown;
}
