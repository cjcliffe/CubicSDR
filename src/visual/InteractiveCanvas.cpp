// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "InteractiveCanvas.h"

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

#include <wx/numformatter.h>

InteractiveCanvas::InteractiveCanvas(wxWindow *parent, int *dispAttrs) :
        wxGLCanvas(parent, wxID_ANY, dispAttrs, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), parent(parent), shiftDown(false), altDown(false), ctrlDown(false), centerFreq(0), bandwidth(0), lastBandwidth(0), isView(
        false) {
    mouseTracker.setTarget(this);
}

InteractiveCanvas::~InteractiveCanvas() {
}

void InteractiveCanvas::setView(long long center_freq_in, long long bandwidth_in) {
    isView = true;
    centerFreq = center_freq_in;
    bandwidth = bandwidth_in;
    lastBandwidth = 0;
}

void InteractiveCanvas::disableView() {
    isView = false;
    centerFreq = wxGetApp().getFrequency();
    bandwidth = wxGetApp().getSampleRate();
    lastBandwidth = 0;
}

bool InteractiveCanvas::getViewState() {
    return isView;
}

long long InteractiveCanvas::getFrequencyAt(float x) {
    long long iqCenterFreq = getCenterFrequency();
    long long iqBandwidth = getBandwidth();
    long long freq = iqCenterFreq - (long long)(0.5 * (long double) iqBandwidth) + ((long double) x * (long double) iqBandwidth);

    return freq;
}

long long InteractiveCanvas::getFrequencyAt(float x, long long iqCenterFreq, long long iqBandwidth) {
    long long freq = iqCenterFreq - (long long)(0.5 * (long double) iqBandwidth) + ((long double) x * (long double) iqBandwidth);
    
    return freq;
}

void InteractiveCanvas::setCenterFrequency(long long center_freq_in) {
    centerFreq = center_freq_in;
}

long long InteractiveCanvas::getCenterFrequency() {
    if (isView) {
        return centerFreq;
    } else {
        return wxGetApp().getFrequency();
    }
}

void InteractiveCanvas::setBandwidth(long long bandwidth_in) {
    bandwidth = bandwidth_in;
}

long long InteractiveCanvas::getBandwidth() {
    if (isView) {
        return bandwidth;
    } else {
        return wxGetApp().getSampleRate();
    }
}

MouseTracker *InteractiveCanvas::getMouseTracker() {
    return &mouseTracker;
}

bool InteractiveCanvas::isAltDown() {
    return altDown;
}

bool InteractiveCanvas::isCtrlDown() {
    return ctrlDown;
}

bool InteractiveCanvas::isShiftDown() {
    return shiftDown;
}

void InteractiveCanvas::OnKeyUp(wxKeyEvent& event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::OnKeyDown(wxKeyEvent& event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::OnMouseMoved(wxMouseEvent& event) {
    mouseTracker.OnMouseMoved(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::OnMouseDown(wxMouseEvent& event) {
    mouseTracker.OnMouseDown(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    mouseTracker.OnMouseWheelMoved(event);
}

void InteractiveCanvas::OnMouseReleased(wxMouseEvent& event) {
    mouseTracker.OnMouseReleased(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    mouseTracker.OnMouseLeftWindow(event);

    shiftDown = false;
    altDown = false;
    ctrlDown = false;
}

void InteractiveCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    mouseTracker.OnMouseEnterWindow(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::setStatusText(std::string statusText) {
    wxGetApp().getAppFrame()->GetStatusBar()->SetStatusText(statusText);
	if (wxGetApp().getConfig()->getShowTips()) {
		if (statusText != lastToolTip) {
			wxToolTip::Enable(false);
			this->SetToolTip(statusText);
			lastToolTip = statusText;
			wxToolTip::SetDelay(1000);
			wxToolTip::Enable(true);
		}
    } else {
        this->SetToolTip("");
        lastToolTip = "";
    }
}

void InteractiveCanvas::setStatusText(std::string statusText, int value) {
    wxGetApp().getAppFrame()->GetStatusBar()->SetStatusText(
            wxString::Format(statusText.c_str(), wxNumberFormatter::ToString((long) value, wxNumberFormatter::Style_WithThousandsSep)));
}

void InteractiveCanvas::OnMouseRightDown(wxMouseEvent& event) {
    mouseTracker.OnMouseRightDown(event);
}

void InteractiveCanvas::OnMouseRightReleased(wxMouseEvent& event) {
    mouseTracker.OnMouseRightReleased(event);
}

bool InteractiveCanvas::isMouseInView() {
    return mouseTracker.mouseInView();
}

bool InteractiveCanvas::isMouseDown() {
    return mouseTracker.mouseInView() && mouseTracker.mouseDown();
}
