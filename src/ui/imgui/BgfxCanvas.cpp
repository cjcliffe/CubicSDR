// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ui/imgui/BgfxCanvas.h"
#include "ui/imgui/imgui_bgfx.h"

#include "ColorTheme.h"
#include "wx/dcclient.h"

#include <cfloat>
#include <chrono>
#include <cstdint>

#if defined(__linux__)
#include <gdk/gdk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif
#endif

namespace {

bool nativeHandles(wxWindow *window, void *&nwh, void *&ndt) {
#if defined(_WIN32)
    nwh = window->GetHandle();
    ndt = nullptr;
    return nwh != nullptr;
#elif defined(__linux__)
    GtkWidget *widget = static_cast<GtkWidget *>(window->GetHandle());
    GdkWindow *gdkWindow = gtk_widget_get_window(widget);
    if (gdkWindow == nullptr) {
        return false;
    }
    GdkDisplay *display = gdk_window_get_display(gdkWindow);
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(display)) {
        nwh = (void *)(uintptr_t)gdk_x11_window_get_xid(gdkWindow);
        ndt = gdk_x11_display_get_xdisplay(display);
        return nwh != nullptr && ndt != nullptr;
    }
#endif
#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(display)) {
        nwh = gdk_wayland_window_get_wl_surface(gdkWindow);
        ndt = gdk_wayland_display_get_wl_display(display);
        return nwh != nullptr && ndt != nullptr;
    }
#endif
    return false;
#elif defined(__APPLE__)
    nwh = window->GetHandle();
    ndt = nullptr;
    return nwh != nullptr;
#else
    nwh = nullptr;
    ndt = nullptr;
    return false;
#endif
}

unsigned char clampByte(float v) {
    if (v < 0.0f) {
        v = 0.0f;
    }
    if (v > 1.0f) {
        v = 1.0f;
    }
    return (unsigned char)(v * 255.0f);
}

} // namespace

bool BgfxCanvas::s_bgfxReady = false;
int BgfxCanvas::s_hosts = 0;
bgfx::ViewId BgfxCanvas::s_nextView = 1;

BgfxCanvas::BgfxCanvas(wxWindow *parent)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_NONE),
      viewId(s_nextView++), fb(BGFX_INVALID_HANDLE), isMainWindow(false), wheelX(0.0f), wheelY(0.0f),
      lastMouseX(-1.0f), lastMouseY(-1.0f), mouseInView(false), leftDown(false), rightDown(false),
      middleDown(false), shiftDown(false), altDown(false), lastPaintS(0.0) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    s_hosts++;
    Bind(wxEVT_PAINT, &BgfxCanvas::OnPaint, this);
    Bind(wxEVT_SIZE, &BgfxCanvas::OnSize, this);
    Bind(wxEVT_MOTION, &BgfxCanvas::OnMotion, this);
    Bind(wxEVT_LEFT_DOWN, &BgfxCanvas::OnButton, this);
    Bind(wxEVT_LEFT_UP, &BgfxCanvas::OnButton, this);
    Bind(wxEVT_LEFT_DCLICK, &BgfxCanvas::OnButton, this);
    Bind(wxEVT_RIGHT_DOWN, &BgfxCanvas::OnButton, this);
    Bind(wxEVT_RIGHT_UP, &BgfxCanvas::OnButton, this);
    Bind(wxEVT_MIDDLE_DOWN, &BgfxCanvas::OnButton, this);
    Bind(wxEVT_MIDDLE_UP, &BgfxCanvas::OnButton, this);
    Bind(wxEVT_MOUSEWHEEL, &BgfxCanvas::OnWheel, this);
    Bind(wxEVT_ENTER_WINDOW, &BgfxCanvas::OnEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &BgfxCanvas::OnLeave, this);
}

BgfxCanvas::~BgfxCanvas() {
    if (bgfx::isValid(fb)) {
        bgfx::destroy(fb);
        fb = BGFX_INVALID_HANDLE;
    }
    s_hosts--;
    if (s_hosts <= 0 && s_bgfxReady) {
        imguiShutdownBackend();
        bgfx::shutdown();
        s_bgfxReady = false;
    }
}

void BgfxCanvas::frameUpdate() {
}

bool BgfxCanvas::isMouseInView() const {
    return mouseInView;
}

bool BgfxCanvas::isShiftDown() const {
    return shiftDown;
}

bool BgfxCanvas::isAltDown() const {
    return altDown;
}

float BgfxCanvas::mouseX() const {
    return lastMouseX;
}

float BgfxCanvas::mouseY() const {
    return lastMouseY;
}

void BgfxCanvas::onEnterView() {
}

void BgfxCanvas::onLeaveView() {
}

void BgfxCanvas::OnPaint(wxPaintEvent &WXUNUSED(event)) {
    wxPaintDC dc(this);
    if (!ensureBackend()) {
        const RGBA4f &bg = ThemeMgr::mgr.currentTheme->generalBackground;
        dc.SetBackground(wxBrush(wxColour(clampByte(bg.r), clampByte(bg.g), clampByte(bg.b))));
        dc.Clear();
        return;
    }
    wxSize size = GetClientSize();
    if (size.x <= 0 || size.y <= 0) {
        return;
    }
    if (!isMainWindow) {
        if (!bgfx::isValid(fb)) {
            void *nwh = nullptr;
            void *ndt = nullptr;
            if (!nativeHandles(this, nwh, ndt)) {
                return;
            }
            double scale = GetContentScaleFactor();
            bgfx::SwapChain desc{};
            desc.width = (uint32_t)(size.x * scale);
            desc.height = (uint32_t)(size.y * scale);
            desc.nwh = nwh;
            desc.ndt = ndt;
            fb = bgfx::createFrameBuffer(desc);
            if (!bgfx::isValid(fb)) {
                return;
            }
        }
        bgfx::setViewFrameBuffer(viewId, fb);
    }
    ImguiInput in;
    in.mx = lastMouseX;
    in.my = lastMouseY;
    in.left = leftDown;
    in.right = rightDown;
    in.middle = middleDown;
    in.wheelX = wheelX;
    in.wheelY = wheelY;
    BgfxEdges edges = pending;
    clearPending();
    double nowS = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    float dt = lastPaintS <= 0.0 ? 1.0f / 60.0f : (float)(nowS - lastPaintS);
    lastPaintS = nowS;
    if (dt > 0.1f) {
        dt = 0.1f;
    }
    imguiBeginFrame(in, size.x, size.y, dt, (float)GetContentScaleFactor());
    drawView(edges, lastMouseX, lastMouseY);
    imguiEndFrame(viewId);
    bgfx::frame();
}

void BgfxCanvas::OnSize(wxSizeEvent &event) {
    event.Skip();
    if (!s_bgfxReady) {
        return;
    }
    wxSize size = GetClientSize() * GetContentScaleFactor();
    if (size.x <= 0 || size.y <= 0) {
        return;
    }
    if (isMainWindow) {
        void *nwh = nullptr;
        void *ndt = nullptr;
        if (nativeHandles(this, nwh, ndt)) {
            bgfx::SwapChain desc{};
            desc.width = (uint32_t)size.x;
            desc.height = (uint32_t)size.y;
            desc.nwh = nwh;
            desc.ndt = ndt;
            bgfx::reset(BGFX_RESET_NONE, &desc);
        }
    } else if (bgfx::isValid(fb)) {
        bgfx::destroy(fb);
        fb = BGFX_INVALID_HANDLE;
    }
    Refresh();
}

void BgfxCanvas::OnMotion(wxMouseEvent &event) {
    updateMods(event);
    lastMouseX = (float)event.GetX();
    lastMouseY = (float)event.GetY();
    mouseInView = true;
    Refresh();
}

void BgfxCanvas::OnButton(wxMouseEvent &event) {
    updateMods(event);
    lastMouseX = (float)event.GetX();
    lastMouseY = (float)event.GetY();
    wxEventType type = event.GetEventType();
    if (type == wxEVT_LEFT_DOWN) {
        leftDown = true;
        pending.leftPressed = true;
        pending.leftClicks = 1;
    } else if (type == wxEVT_LEFT_DCLICK) {
        leftDown = true;
        pending.leftPressed = true;
        pending.leftClicks = 2;
    } else if (type == wxEVT_LEFT_UP) {
        leftDown = false;
        pending.leftReleased = true;
    } else if (type == wxEVT_RIGHT_DOWN) {
        rightDown = true;
        pending.rightPressed = true;
    } else if (type == wxEVT_RIGHT_UP) {
        rightDown = false;
        pending.rightReleased = true;
    } else if (type == wxEVT_MIDDLE_DOWN) {
        middleDown = true;
        pending.middlePressed = true;
    } else if (type == wxEVT_MIDDLE_UP) {
        middleDown = false;
    }
    Refresh();
}

void BgfxCanvas::OnWheel(wxMouseEvent &event) {
    updateMods(event);
    int delta = event.GetWheelDelta();
    if (delta == 0) {
        delta = 120;
    }
    int rot = event.GetWheelRotation() * 120 / delta;
    if (event.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL) {
        pending.wheelRotH += rot;
        wheelX += (float)rot / 120.0f;
    } else {
        pending.wheelRotV += rot;
        wheelY += (float)rot / 120.0f;
    }
    Refresh();
}

void BgfxCanvas::OnEnter(wxMouseEvent &event) {
    updateMods(event);
    mouseInView = true;
    onEnterView();
    Refresh();
}

void BgfxCanvas::OnLeave(wxMouseEvent &event) {
    updateMods(event);
    pending.mouseLeft = true;
    leftDown = false;
    rightDown = false;
    middleDown = false;
    mouseInView = false;
    lastMouseX = -FLT_MAX;
    lastMouseY = -FLT_MAX;
    onLeaveView();
    Refresh();
}

bool BgfxCanvas::ensureBackend() {
    if (s_bgfxReady) {
        return true;
    }
    void *nwh = nullptr;
    void *ndt = nullptr;
    if (!nativeHandles(this, nwh, ndt)) {
        return false;
    }
    wxSize size = GetClientSize();
    double scale = GetContentScaleFactor();
    int pw = (int)(size.x * scale);
    int ph = (int)(size.y * scale);
    if (pw <= 0 || ph <= 0) {
        return false;
    }
    bgfx::Init init;
#if defined(__APPLE__)
    init.type = bgfx::RendererType::Metal;
#else
    init.type = bgfx::RendererType::OpenGL;
#endif
    init.reset = BGFX_RESET_NONE;
    init.swapChain.width = (uint32_t)pw;
    init.swapChain.height = (uint32_t)ph;
    init.swapChain.nwh = nwh;
    init.swapChain.ndt = ndt;
    if (!bgfx::init(init)) {
        return false;
    }
    if (!imguiInitBackend()) {
        bgfx::shutdown();
        return false;
    }
    isMainWindow = true;
    s_bgfxReady = true;
    return true;
}

void BgfxCanvas::updateMods(const wxMouseEvent &event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
}

void BgfxCanvas::clearPending() {
    pending = BgfxEdges();
    wheelX = 0.0f;
    wheelY = 0.0f;
}
