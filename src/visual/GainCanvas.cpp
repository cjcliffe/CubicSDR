#include "GainCanvas.h"

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

wxBEGIN_EVENT_TABLE(GainCanvas, wxGLCanvas) EVT_PAINT(GainCanvas::OnPaint)
EVT_IDLE(GainCanvas::OnIdle)
EVT_MOTION(GainCanvas::OnMouseMoved)
EVT_LEFT_DOWN(GainCanvas::OnMouseDown)
EVT_LEFT_UP(GainCanvas::OnMouseReleased)
EVT_LEAVE_WINDOW(GainCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(GainCanvas::OnMouseEnterWindow)
EVT_MOUSEWHEEL(GainCanvas::OnMouseWheelMoved)
wxEND_EVENT_TABLE()

GainCanvas::GainCanvas(wxWindow *parent, int *dispAttrs) :
        InteractiveCanvas(parent, dispAttrs) {

    glContext = new PrimaryGLContext(this, &wxGetApp().GetContext(this));
    bgPanel.setCoordinateSystem(GLPanel::GLPANEL_Y_UP);
    bgPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_X);

    numGains = 1;
    spacing = 2.0/numGains;
    barWidth = (1.0/numGains)*0.8;
    startPos = spacing/2.0;
    barHeight = 0.8f;
    refreshCounter = 0;
}

GainCanvas::~GainCanvas() {

}

void GainCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    initGLExtensions();

    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    bgPanel.draw();
    
    SwapBuffers();
}

void GainCanvas::OnIdle(wxIdleEvent &event) {
	if (mouseTracker.mouseInView()) {
	    Refresh();
	} else {
		event.Skip();
	}
    
    for (auto gi : gainPanels) {
        if (gi->getChanged()) {
            wxGetApp().setGain(gi->getName(), gi->getValue());
            gi->setChanged(false);
        }
    }
}

void GainCanvas::SetLevel() {
    CubicVR::vec2 mpos = mouseTracker.getGLXY();
    
    for (auto gi : gainPanels) {
        if (gi->isMeterHit(mpos)) {
            float value = gi->getMeterHitValue(mpos, *gi);
            
            gi->setValue(value);
            gi->setChanged(true);
            
            break;
        }
    }
}

void GainCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);

    CubicVR::vec2 mpos = mouseTracker.getGLXY();
    
    for (auto gi : gainPanels) {
        if (gi->isMeterHit(mpos)) {
            float value = gi->getMeterHitValue(mpos, *gi);
        
            gi->setHighlight(value);
            gi->setHighlightVisible(true);
            wxGetApp().setActiveGainEntry(gi->getName());
        } else {
            gi->setHighlightVisible(false);
        }
    }
    
    if (mouseTracker.mouseDown()) {
        SetLevel();
    }
}

void GainCanvas::OnMouseDown(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseDown(event);
    SetLevel();
}

void GainCanvas::OnMouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseWheelMoved(event);
    
    CubicVR::vec2 hitResult;
    
    CubicVR::vec2 mpos = mouseTracker.getGLXY();
    
    for (auto gi : gainPanels) {
        if (gi->isMeterHit(mpos)) {
            float movement = 3.0 * (float)event.GetWheelRotation();
            gi->setValue(gi->getValue() + ((movement / 100.0) * ((gi->getHigh() - gi->getLow()) / 100.0)));
            gi->setChanged(true);
            break;
        }
    }
}

void GainCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
}

void GainCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
   
    for (auto gi : gainPanels) {
        gi->setHighlightVisible(false);
    }
    
    Refresh();
}

void GainCanvas::OnMouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseTracker.OnMouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
#ifdef _WIN32
	if (wxGetApp().getAppFrame()->canFocus()) {
		this->SetFocus();
	}
#endif
}



void GainCanvas::setHelpTip(std::string tip) {
    helpTip = tip;
}

void GainCanvas::updateGainUI() {
    SDRDeviceInfo *devInfo = wxGetApp().getDevice();
    DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(devInfo->getDeviceId());
    
    gains = devInfo->getGains(SOAPY_SDR_RX, 0);
    SDRRangeMap::iterator gi;
    
    numGains = gains.size();
    float i = 0;
    
    if (!numGains) {
        return;
    }
    
    spacing = 2.0/numGains;
    barWidth = (1.0/numGains)*0.7;
    startPos = spacing/2.0;
    barHeight = 1.0f;
    
    while (gainPanels.size()) {
        MeterPanel *mDel = gainPanels.back();
        gainPanels.pop_back();
        bgPanel.removeChild(mDel);
        delete mDel;
    }
    
    for (auto gi : gains) {
        MeterPanel *mPanel = new MeterPanel(gi.first, gi.second.minimum(), gi.second.maximum(), devConfig->getGain(gi.first,wxGetApp().getGain(gi.first)));

        float midPos = -1.0+startPos+spacing*i;
        mPanel->setPosition(midPos, 0);
        mPanel->setSize(barWidth, barHeight);
        bgPanel.addChild(mPanel);
        
        gainPanels.push_back(mPanel);
        i++;
    }
    
    setThemeColors();
}

void GainCanvas::setThemeColors() {
    RGBA4f c1, c2;
    
    c1 = ThemeMgr::mgr.currentTheme->generalBackground;
    c2 = ThemeMgr::mgr.currentTheme->generalBackground * 0.5;
    c1.a = 1.0;
    c2.a = 1.0;
    bgPanel.setFillColor(c1, c2);

    Refresh();
}

