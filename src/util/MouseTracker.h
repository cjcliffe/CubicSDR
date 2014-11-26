#pragma once

#include "wx/window.h"

class MouseTracker {
public:
    MouseTracker(wxWindow *target) :
            target(target), mouseX(0), mouseY(0), lastMouseX(0), lastMouseY(0), originMouseX(0), originMouseY(0), deltaMouseX(0), deltaMouseY(0), isMouseDown(
                    false), vertDragLock(false), isMouseInView(false) {

    }

    MouseTracker() :
            target(NULL), mouseX(0), mouseY(0), lastMouseX(0), lastMouseY(0), originMouseX(0), originMouseY(0), deltaMouseX(0), deltaMouseY(0), isMouseDown(
                    false), vertDragLock(false), isMouseInView(false) {

    }

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    float getOriginMouseX();
    float getOriginMouseY();
    float getOriginDeltaMouseX();
    float getOriginDeltaMouseY();
    float getDeltaMouseX();
    float getDeltaMouseY();
    float getLastMouseX();
    float getLastMouseY();
    float getMouseX();
    float getMouseY();

    void setVertDragLock(bool dragLock);
    bool mouseDown();
    bool mouseInView();
    void setTarget(wxWindow *target_in);

private:
    float mouseX, mouseY;
    float lastMouseX, lastMouseY;
    float deltaMouseX, deltaMouseY;
    float originMouseX, originMouseY;

    bool isMouseDown, isMouseInView;
    bool vertDragLock;
    wxWindow *target;
};
