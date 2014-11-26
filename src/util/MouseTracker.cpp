#include "MouseTracker.h"
#include <iostream>

void MouseTracker::OnMouseMoved(wxMouseEvent& event) {
    if (target == NULL) {
        return;
    }

    const wxSize ClientSize = target->GetClientSize();

    mouseX = (float) event.m_x / (float) ClientSize.x;
    mouseY = (float) event.m_y / (float) ClientSize.y;

    deltaMouseX = mouseX - lastMouseX;
    deltaMouseY = mouseY - lastMouseY;

    if (isMouseDown) {
        lastMouseX = mouseX;

        if (vertDragLock && mouseY != lastMouseY) {
            target->WarpPointer(event.m_x, lastMouseY * ClientSize.y);
        } else {
            lastMouseY = mouseY;
        }
    } else {
        lastMouseY = mouseY;
        lastMouseX = mouseX;
    }
}

void MouseTracker::OnMouseDown(wxMouseEvent& event) {
    if (target == NULL) {
        return;
    }

    const wxSize ClientSize = target->GetClientSize();

    mouseX = lastMouseX = (float) event.m_x / (float) ClientSize.x;
    mouseY = lastMouseY = (float) event.m_y / (float) ClientSize.y;

    originMouseX = mouseX;
    originMouseY = mouseY;

    isMouseDown = true;
}

void MouseTracker::OnMouseWheelMoved(wxMouseEvent& event) {
//    std::cout << "wheel?" << std::endl;
}

void MouseTracker::OnMouseReleased(wxMouseEvent& event) {
    isMouseDown = false;
}

void MouseTracker::OnMouseEnterWindow(wxMouseEvent& event) {
    isMouseInView = true;
}

void MouseTracker::OnMouseLeftWindow(wxMouseEvent& event) {
    isMouseDown = false;
    isMouseInView = false;
}

float MouseTracker::getOriginMouseX() {
    return originMouseX;
}

float MouseTracker::getOriginMouseY() {
    return originMouseY;
}

float MouseTracker::getOriginDeltaMouseX() {
    return mouseX - originMouseX;
}

float MouseTracker::getOriginDeltaMouseY() {
    return mouseY - originMouseY;
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

bool MouseTracker::mouseInView() {
    return isMouseInView;
}

void MouseTracker::setTarget(wxWindow *target_in) {
    target = target_in;
}
