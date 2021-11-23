// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/window.h"
#include "cubic_math.h"

class MouseTracker {
public:
    MouseTracker(wxWindow *target);
    MouseTracker();

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseRightDown(wxMouseEvent& event);
    void OnMouseRightReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    float getOriginMouseX() const;
    float getOriginMouseY() const;
    float getOriginDeltaMouseX() const;
    float getOriginDeltaMouseY() const;
    float getDeltaMouseX() const;
    float getDeltaMouseY() const;
    float getLastMouseX() const;
    float getLastMouseY() const;
    CubicVR::vec2 getGLXY() const;
    float getMouseX() const;
    float getMouseY() const;

    void setVertDragLock(bool dragLock);
    void setHorizDragLock(bool dragLock);
    bool getVertDragLock() const;
    bool getHorizDragLock() const;
    bool mouseDown() const;
    bool mouseRightDown() const;
    bool mouseInView() const;
    void setTarget(wxWindow *target_in);

private:
    float mouseX, mouseY;
    float lastMouseX, lastMouseY;
    float originMouseX, originMouseY;
    float deltaMouseX, deltaMouseY;

    bool vertDragLock, horizDragLock;
    bool isMouseDown, isMouseRightDown, isMouseInView;
    wxWindow *target;
};
