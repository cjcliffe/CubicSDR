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

    float i = 0;
    for (std::vector<GainInfo *>::iterator gi = gainInfo.begin(); gi != gainInfo.end(); gi++) {
        GainInfo *gInfo = (*gi);
        float midPos = -1.0+startPos+spacing*i;

        gInfo->labelPanel.setSize(spacing/2.0,(15.0/float(ClientSize.y)));
        gInfo->labelPanel.setPosition(midPos, -barHeight-(20.0/float(ClientSize.y)));
        
        gInfo->valuePanel.setSize(spacing/2.0,(15.0/float(ClientSize.y)));
        gInfo->valuePanel.setPosition(midPos, barHeight+(20.0/float(ClientSize.y)));
        
        i+=1.0;
    }

    bgPanel.draw();
    
    SwapBuffers();
}

void GainCanvas::OnIdle(wxIdleEvent &event) {
	if (mouseTracker.mouseInView()) {
	    Refresh();
	} else {
		event.Skip();
	}
    
    for (std::vector<GainInfo *>::iterator gi = gainInfo.begin(); gi != gainInfo.end(); gi++) {
        GainInfo *gInfo = (*gi);
        if (gInfo->changed) {
            wxGetApp().setGain(gInfo->name, gInfo->current);
            gInfo->changed = false;
        }
    }
}

int GainCanvas::GetPanelHit(CubicVR::vec2 &result) {
    std::vector<GainInfo *>::iterator gi;

    int i = 0;
    for (gi = gainInfo.begin(); gi != gainInfo.end(); gi++) {
        GainInfo *gInfo = (*gi);
        
        CubicVR::vec2 hitResult;
        if (gInfo->panel.hitTest(CubicVR::vec2((mouseTracker.getMouseX()-0.5)*2.0, (mouseTracker.getMouseY()-0.5)*2.0), hitResult)) {
//            std::cout << "Hit #" << i << " result: " << hitResult << std::endl;
            result = (hitResult + CubicVR::vec2(1.0,1.0)) * 0.5;
            return i;
        }
        i++;
    }
    return -1;
}


void GainCanvas::SetLevel() {
    CubicVR::vec2 hitResult;
    int panelHit = GetPanelHit(hitResult);
    
    if (panelHit >= 0) {
        gainInfo[panelHit]->levelPanel.setSize(1.0, hitResult.y);
        gainInfo[panelHit]->levelPanel.setPosition(0.0, (-1.0+(hitResult.y)));
        gainInfo[panelHit]->current = round(gainInfo[panelHit]->low+(hitResult.y * (gainInfo[panelHit]->high-gainInfo[panelHit]->low)));
        gainInfo[panelHit]->changed = true;
        gainInfo[panelHit]->valuePanel.setText(std::to_string(int(gainInfo[panelHit]->current)),GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, true);
    }
}

void GainCanvas::OnMouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseMoved(event);

    CubicVR::vec2 hitResult;
    int panelHit = GetPanelHit(hitResult);
    
    if (panelHit >= 0) {
        gainInfo[panelHit]->highlightPanel.setSize(1.0, hitResult.y);
        gainInfo[panelHit]->highlightPanel.setPosition(0.0, (-1.0+(hitResult.y)));
    }
    
    int i = 0;
    for (std::vector<GainInfo *>::iterator gi = gainInfo.begin(); gi != gainInfo.end(); gi++) {
        (*gi)->highlightPanel.visible = (i==panelHit);
        if (i==panelHit) {
            wxGetApp().setActiveGainEntry((*gi)->name);
        }
        i++;
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
    int panelHit = GetPanelHit(hitResult);

    if (panelHit >= 0) {
        float movement = 3.0 * (float)event.GetWheelRotation();
        
        GainInfo *gInfo;
        
        gInfo = gainInfo[panelHit];
        
        gInfo->current = gInfo->current + ((movement / 100.0) * ((gInfo->high - gInfo->low) / 100.0));
        
        //BEGIN Clamp to prevent the meter to escape
        if (gInfo->current > gInfo->high) {
            gInfo->current = gInfo->high;
        }
        if (gInfo->current < gInfo->low) {
            gInfo->current = gInfo->low;
        }
       
        gInfo->changed = true;
        
        float levelVal = float(gInfo->current-gInfo->low)/float(gInfo->high-gInfo->low);
        gInfo->levelPanel.setSize(1.0, levelVal);
        gInfo->levelPanel.setPosition(0.0, levelVal-1.0);

        gInfo->valuePanel.setText(std::to_string(int(gInfo->current)),GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, true);
    }

}

void GainCanvas::OnMouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseReleased(event);
}

void GainCanvas::OnMouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::OnMouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
    
    int i = 0;
    for (std::vector<GainInfo *>::iterator gi = gainInfo.begin(); gi != gainInfo.end(); gi++) {
        (*gi)->highlightPanel.visible = false;
        i++;
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
    const wxSize ClientSize = GetClientSize();

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
    barHeight = 0.8f;
    
    RGBA4f c1, c2;
    
    while (gainInfo.size()) {
        GainInfo *giDel;
        giDel = gainInfo.back();
        gainInfo.pop_back();
        
        giDel->panel.removeChild(&giDel->levelPanel);
        bgPanel.removeChild(&(giDel->labelPanel));
        bgPanel.removeChild(&(giDel->valuePanel));
        bgPanel.removeChild(&(giDel->panel));
        delete giDel;
    }
    
    for (gi = gains.begin(); gi != gains.end(); gi++) {
        GainInfo *gInfo = new GainInfo;
        float midPos = -1.0+startPos+spacing*i;
        
        gInfo->name = gi->first;
        gInfo->low = gi->second.minimum();
        gInfo->high = gi->second.maximum();
        gInfo->current = devConfig->getGain(gInfo->name,wxGetApp().getGain(gInfo->name));
        gInfo->changed = false;
        
        gInfo->panel.setBorderPx(1);
        gInfo->panel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
        gInfo->panel.setPosition(midPos, 0);
        gInfo->panel.setSize(barWidth, barHeight);
        gInfo->panel.setBlend(GL_ONE, GL_ONE);

        gInfo->levelPanel.setBorderPx(0);
        gInfo->levelPanel.setMarginPx(1);
        gInfo->levelPanel.setSize(1.0,0.8f);
        float levelVal = float(gInfo->current-gInfo->low)/float(gInfo->high-gInfo->low);
        gInfo->levelPanel.setSize(1.0, levelVal);
        gInfo->levelPanel.setPosition(0.0, (-1.0+(levelVal)));
        gInfo->levelPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
        gInfo->levelPanel.setBlend(GL_ONE, GL_ONE);

        gInfo->panel.addChild(&gInfo->levelPanel);

        gInfo->highlightPanel.setBorderPx(0);
        gInfo->highlightPanel.setMarginPx(1);
        gInfo->highlightPanel.setSize(1.0,0.8f);
        gInfo->highlightPanel.setPosition(0.0,-0.2f);
        gInfo->highlightPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_BAR_X);
        gInfo->highlightPanel.setBlend(GL_ONE, GL_ONE);
        gInfo->highlightPanel.visible = false;
        
        gInfo->panel.addChild(&gInfo->highlightPanel);
        
        gInfo->labelPanel.setSize(spacing/2.0,(15.0/float(ClientSize.y)));
        gInfo->labelPanel.setPosition(midPos, -barHeight-(20.0/float(ClientSize.y)));

        gInfo->labelPanel.setText(gi->first,GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, true);
        gInfo->labelPanel.setFill(GLPanel::GLPANEL_FILL_NONE);
        
        bgPanel.addChild(&(gInfo->labelPanel));
        
        gInfo->valuePanel.setSize(spacing/2.0,(15.0/float(ClientSize.y)));
        gInfo->valuePanel.setPosition(midPos, barHeight+(20.0/float(ClientSize.y)));
        
        gInfo->valuePanel.setText(std::to_string(int(gInfo->current)), GLFont::GLFONT_ALIGN_CENTER, GLFont::GLFONT_ALIGN_CENTER, true);
        gInfo->valuePanel.setFill(GLPanel::GLPANEL_FILL_NONE);
        
        bgPanel.addChild(&(gInfo->valuePanel));
        
        bgPanel.addChild(&(gInfo->panel));
        gainInfo.push_back(gInfo);
        i++;
    }
    
    setThemeColors();
}

void GainCanvas::setThemeColors() {
    std::vector<GainInfo *>::iterator gi;

    RGBA4f c1, c2;
    
    c1 = ThemeMgr::mgr.currentTheme->generalBackground;
    c2 = ThemeMgr::mgr.currentTheme->generalBackground * 0.5;
    
    bgPanel.setFillColor(c1, c2);

    for (gi = gainInfo.begin(); gi != gainInfo.end(); gi++) {
        GainInfo *gInfo = (*gi);
        
        c1 = ThemeMgr::mgr.currentTheme->generalBackground;
        c2 = ThemeMgr::mgr.currentTheme->generalBackground * 0.5;
        c1.a = 1.0;
        c2.a = 1.0;
        gInfo->panel.setFillColor(c1, c2);

        c1 = ThemeMgr::mgr.currentTheme->meterLevel * 0.5;
        c2 = ThemeMgr::mgr.currentTheme->meterLevel;
        c1.a = 1.0;
        c2.a = 1.0;
        gInfo->levelPanel.setFillColor(c1, c2);
        
        c1 = RGBA4f(0.3f,0.3f,0.3f,1.0f);
        c2 = RGBA4f(0.65f,0.65f,0.65f,1.0f);;
        gInfo->highlightPanel.setFillColor(c1, c2);
    }
    Refresh();
}

