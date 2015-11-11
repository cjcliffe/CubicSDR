#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "MouseTracker.h"
#include "GLPanel.h"
#include "PrimaryGLContext.h"

#include "fftw3.h"
#include "Timer.h"

class GainInfo {
public:
    std::string name;
    float low, high, current;
    bool changed;
    GLPanel panel;
    GLPanel levelPanel;
    GLPanel highlightPanel;
    GLTextPanel labelPanel;
    GLTextPanel valuePanel;
};

class GainCanvas: public InteractiveCanvas {
public:
    GainCanvas(wxWindow *parent, int *attribList = NULL);
    ~GainCanvas();

    void setHelpTip(std::string tip);
    void updateGainUI();
    void setThemeColors();
    
private:
    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    int GetPanelHit(CubicVR::vec2 &result);
    void SetLevel();

    void OnShow(wxShowEvent& event);
    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    PrimaryGLContext *glContext;
    std::string helpTip;
    std::vector<GainInfo *> gainInfo;
    GLPanel bgPanel;
    
    float spacing, barWidth, startPos, barHeight, numGains;
    wxSize clientSize;
    //
wxDECLARE_EVENT_TABLE();
};

