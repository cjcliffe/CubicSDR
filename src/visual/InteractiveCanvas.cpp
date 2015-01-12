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

InteractiveCanvas::InteractiveCanvas(wxWindow *parent, int *attribList) :
        wxGLCanvas(parent, wxID_ANY, attribList, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE), parent(parent), shiftDown(false), altDown(false), ctrlDown(false), centerFreq(0), bandwidth(0), lastBandwidth(0), isView(
        false) {
    mouseTracker.setTarget(this);
}

InteractiveCanvas::~InteractiveCanvas() {
}

void InteractiveCanvas::setView(long long center_freq_in, int bandwidth_in) {
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

long long InteractiveCanvas::getFrequencyAt(float x) {
    long long iqCenterFreq = getCenterFrequency();
    long long iqBandwidth = getBandwidth();
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

void InteractiveCanvas::setBandwidth(unsigned int bandwidth_in) {
    bandwidth = bandwidth_in;
}

unsigned int InteractiveCanvas::getBandwidth() {
    if (isView) {
        return bandwidth;
    } else {
        return wxGetApp().getSampleRate();
    }
}

void InteractiveCanvas::OnKeyUp(wxKeyEvent& event) {
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::OnKeyDown(wxKeyEvent& event) {
    float angle = 5.0;

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
}

void InteractiveCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    mouseTracker.OnMouseEnterWindow(event);
}

void InteractiveCanvas::setStatusText(std::string statusText) {
    ((wxFrame*) parent)->GetStatusBar()->SetStatusText(statusText);
}

void InteractiveCanvas::setStatusText(std::string statusText, int value) {
    ((wxFrame*) parent)->GetStatusBar()->SetStatusText(
            wxString::Format(statusText.c_str(), wxNumberFormatter::ToString((long) value, wxNumberFormatter::Style_WithThousandsSep)));
}

void InteractiveCanvas::OnMouseRightDown(wxMouseEvent& event) {
    mouseTracker.OnMouseRightDown(event);
}

void InteractiveCanvas::OnMouseRightReleased(wxMouseEvent& event) {
    mouseTracker.OnMouseRightReleased(event);
}
