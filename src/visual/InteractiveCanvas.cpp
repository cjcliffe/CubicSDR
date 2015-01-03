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
        wxFULL_REPAINT_ON_RESIZE), parent(parent), shiftDown(false), altDown(false), ctrlDown(false), center_freq(0), bandwidth(0), last_bandwidth(0), isView(
        false) {
    mTracker.setTarget(this);
}

InteractiveCanvas::~InteractiveCanvas() {
}

void InteractiveCanvas::SetView(int center_freq_in, int bandwidth_in) {
    isView = true;
    center_freq = center_freq_in;
    bandwidth = bandwidth_in;
    last_bandwidth = 0;
}

void InteractiveCanvas::DisableView() {
    isView = false;
    center_freq = wxGetApp().getFrequency();
    bandwidth = SRATE;
    last_bandwidth = 0;
}

int InteractiveCanvas::GetFrequencyAt(float x) {
    int iqCenterFreq = GetCenterFrequency();
    int iqBandwidth = GetBandwidth();
    int freq = iqCenterFreq - (int) (0.5 * (float) iqBandwidth) + (int) ((float) x * (float) iqBandwidth);

    return freq;
}

void InteractiveCanvas::SetCenterFrequency(unsigned int center_freq_in) {
    center_freq = center_freq_in;
}

unsigned int InteractiveCanvas::GetCenterFrequency() {
    if (isView) {
        return center_freq;
    } else {
        return (unsigned int) wxGetApp().getFrequency();
    }
}

void InteractiveCanvas::SetBandwidth(unsigned int bandwidth_in) {
    bandwidth = bandwidth_in;
}

unsigned int InteractiveCanvas::GetBandwidth() {
    if (isView) {
        return bandwidth;
    } else {
        return SRATE;
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

void InteractiveCanvas::mouseMoved(wxMouseEvent& event) {
    mTracker.OnMouseMoved(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::mouseDown(wxMouseEvent& event) {
    mTracker.OnMouseDown(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::mouseWheelMoved(wxMouseEvent& event) {
    mTracker.OnMouseWheelMoved(event);
}

void InteractiveCanvas::mouseReleased(wxMouseEvent& event) {
    mTracker.OnMouseReleased(event);

    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
}

void InteractiveCanvas::mouseLeftWindow(wxMouseEvent& event) {
    mTracker.OnMouseLeftWindow(event);
}

void InteractiveCanvas::mouseEnterWindow(wxMouseEvent& event) {
    mTracker.OnMouseEnterWindow(event);
}

void InteractiveCanvas::setStatusText(std::string statusText) {
    ((wxFrame*) parent)->GetStatusBar()->SetStatusText(statusText);
}

void InteractiveCanvas::setStatusText(std::string statusText, int value) {
    ((wxFrame*) parent)->GetStatusBar()->SetStatusText(
            wxString::Format(statusText.c_str(), wxNumberFormatter::ToString((long) value, wxNumberFormatter::Style_WithThousandsSep)));
}
