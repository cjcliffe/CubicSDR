#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include "MouseTracker.h"

class InteractiveCanvas: public wxGLCanvas {
public:
    InteractiveCanvas(wxWindow *parent, int *attribList = NULL);
    ~InteractiveCanvas();

    int GetFrequencyAt(float x);

    void SetView(int center_freq_in, int bandwidth_in);
    void DisableView();

    void SetCenterFrequency(unsigned int center_freq_in);
    unsigned int GetCenterFrequency();

    void SetBandwidth(unsigned int bandwidth_in);
    unsigned int GetBandwidth();

protected:
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void mouseEnterWindow(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);

    wxWindow *parent;
    MouseTracker mTracker;

    bool shiftDown;
    bool altDown;
    bool ctrlDown;

    unsigned int center_freq;
    unsigned int bandwidth;
    unsigned int last_bandwidth;

    bool isView;
};

