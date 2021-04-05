// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "MouseTracker.h"

MouseTracker::MouseTracker(wxWindow *target) :
        mouseX(0), mouseY(0), lastMouseX(0), lastMouseY(0), originMouseX(0), originMouseY(0), deltaMouseX(0), deltaMouseY(0), vertDragLock(false), horizDragLock(
                false), isMouseDown(false), isMouseRightDown(false), isMouseInView(false), target(target) {

}

MouseTracker::MouseTracker() :
        MouseTracker(nullptr) {

}

void MouseTracker::OnMouseMoved(wxMouseEvent& event) {
    if (target == nullptr) {
        return;
    }

    const wxSize ClientSize = target->GetClientSize();

    mouseX = (float) event.m_x / (float) ClientSize.x;
    mouseY = 1.0 - (float) event.m_y / (float) ClientSize.y;

    deltaMouseX = mouseX - lastMouseX;
    deltaMouseY = mouseY - lastMouseY;

    if (isMouseDown || isMouseRightDown) {
#ifndef __APPLE__
#if !defined(__linux__) && !defined(__FreeBSD__)
        if (horizDragLock && vertDragLock) {
            target->WarpPointer(originMouseX * ClientSize.x, (1.0 - originMouseY) * ClientSize.y);
            mouseX = originMouseX;
            mouseY = originMouseY;
        } else if (vertDragLock && mouseY != lastMouseY) {
            target->WarpPointer(event.m_x, (1.0 - originMouseY) * ClientSize.y);
            mouseY = originMouseY;
        } else if (horizDragLock && mouseX != lastMouseX) {
            target->WarpPointer(originMouseX * ClientSize.x, event.m_y);
            mouseX = originMouseX;
        }
#endif
#endif
    }

    lastMouseX = mouseX;
    lastMouseY = mouseY;
}

void MouseTracker::OnMouseWheelMoved(wxMouseEvent& /* event */) {
//    std::cout << "wheel?" << std::endl;
}

void MouseTracker::OnMouseDown(wxMouseEvent& event) {
    if (isMouseRightDown || target == nullptr) {
        return;
    }

    const wxSize ClientSize = target->GetClientSize();

    mouseX = lastMouseX = (float) event.m_x / (float) ClientSize.x;
    mouseY = lastMouseY = 1.0 - (float) event.m_y / (float) ClientSize.y;

    originMouseX = mouseX;
    originMouseY = mouseY;

    isMouseDown = true;
}

void MouseTracker::OnMouseReleased(wxMouseEvent& /* event */) {
    isMouseDown = false;
}

void MouseTracker::OnMouseRightDown(wxMouseEvent& event) {
    if (isMouseDown || target == nullptr) {
        return;
    }

    const wxSize ClientSize = target->GetClientSize();

    mouseX = lastMouseX = (float) event.m_x / (float) ClientSize.x;
    mouseY = lastMouseY = 1.0 - (float) event.m_y / (float) ClientSize.y;

    originMouseX = mouseX;
    originMouseY = mouseY;

    isMouseRightDown = true;
}

void MouseTracker::OnMouseRightReleased(wxMouseEvent& /* event */) {
    isMouseRightDown = false;
}

void MouseTracker::OnMouseEnterWindow(wxMouseEvent& /* event */) {
    isMouseInView = true;
    isMouseDown = false;
    isMouseRightDown = false;
}

void MouseTracker::OnMouseLeftWindow(wxMouseEvent& /* event */) {
    isMouseDown = false;
    isMouseRightDown = false;
    isMouseInView = false;
}

float MouseTracker::getOriginMouseX() const {
    return originMouseX;
}

float MouseTracker::getOriginMouseY() const {
    return originMouseY;
}

float MouseTracker::getOriginDeltaMouseX() const {
    return mouseX - originMouseX;
}

float MouseTracker::getOriginDeltaMouseY() const {
    return mouseY - originMouseY;
}

float MouseTracker::getDeltaMouseX() const {
    return deltaMouseX;
}

float MouseTracker::getDeltaMouseY() const {
    return deltaMouseY;
}

float MouseTracker::getLastMouseX() const {
    return lastMouseX;
}

float MouseTracker::getLastMouseY() const {
    return lastMouseY;
}

CubicVR::vec2 MouseTracker::getGLXY() const {
    return CubicVR::vec2((getMouseX()-0.5)*2.0, (getMouseY()-0.5)*2.0);
}

float MouseTracker::getMouseX() const {
    return mouseX;
}

float MouseTracker::getMouseY() const {
    return mouseY;
}

void MouseTracker::setVertDragLock(bool dragLock) {
    vertDragLock = dragLock;
}

void MouseTracker::setHorizDragLock(bool dragLock) {
    horizDragLock = dragLock;
}

bool MouseTracker::getVertDragLock() const {
    return vertDragLock;
}

bool MouseTracker::getHorizDragLock() const {
    return horizDragLock;
}

bool MouseTracker::mouseDown() const {
    return isMouseDown;
}

bool MouseTracker::mouseInView() const {
    return isMouseInView;
}

void MouseTracker::setTarget(wxWindow *target_in) {
    target = target_in;
}


bool MouseTracker::mouseRightDown() const {
    return isMouseRightDown;
}
