// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/window.h"

#include <bgfx/bgfx.h>

struct BgfxEdges {
    bool leftPressed = false;
    int leftClicks = 0;
    bool leftReleased = false;
    bool rightPressed = false;
    bool rightReleased = false;
    bool middlePressed = false;
    bool middleReleased = false;
    int wheelRotV = 0;
    int wheelRotH = 0;
    bool mouseLeft = false;
};

class BgfxCanvas : public wxWindow {
public:
    BgfxCanvas(wxWindow *parent);
    ~BgfxCanvas() override;

    static bool isBackendReady();

    virtual void frameUpdate();

    bool isMouseInView() const;
    bool isShiftDown() const;
    bool isAltDown() const;

protected:
    virtual void drawView(const BgfxEdges &edges, float mx, float my) = 0;
    virtual void onEnterView();
    virtual void onLeaveView();

    float mouseX() const;
    float mouseY() const;

private:
    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);
    void OnMotion(wxMouseEvent &event);
    void OnButton(wxMouseEvent &event);
    void OnWheel(wxMouseEvent &event);
    void OnEnter(wxMouseEvent &event);
    void OnLeave(wxMouseEvent &event);

    bool ensureBackend();
    void updateMods(const wxMouseEvent &event);
    void clearPending();

    static bool s_bgfxReady;
    static int s_hosts;
    static bgfx::ViewId s_nextView;

    bgfx::ViewId viewId;
    bgfx::FrameBufferHandle fb;
    bool isMainWindow;

    BgfxEdges pending;
    float wheelX;
    float wheelY;
    float lastMouseX;
    float lastMouseY;
    bool mouseInView;
    bool leftDown;
    bool rightDown;
    bool middleDown;
    bool shiftDown;
    bool altDown;
    double lastPaintS;
    wxSize lastSize;
};
