// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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
#include <cmath>

wxBEGIN_EVENT_TABLE(GainCanvas, wxGLCanvas) EVT_PAINT(GainCanvas::OnPaint)
EVT_IDLE(GainCanvas::OnIdle)
EVT_MOTION(GainCanvas::OnMouseMoved)
EVT_LEFT_DOWN(GainCanvas::OnMouseDown)
EVT_LEFT_UP(GainCanvas::OnMouseReleased)
EVT_LEAVE_WINDOW(GainCanvas::OnMouseLeftWindow)
EVT_ENTER_WINDOW(GainCanvas::OnMouseEnterWindow)
EVT_MOUSEWHEEL(GainCanvas::OnMouseWheelMoved)
wxEND_EVENT_TABLE()

GainCanvas::GainCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs) :
        InteractiveCanvas(parent, dispAttrs) {

    glContext = new PrimaryGLContext(this, &wxGetApp().GetContext(this), wxGetApp().GetContextAttributes());
    bgPanel.setCoordinateSystem(GLPanel::GLPANEL_Y_UP);
    bgPanel.setFill(GLPanel::GLPANEL_FILL_GRAD_X);

    numGains = 1;
    spacing = 2.0/numGains;
    barWidth = (1.0/numGains)*0.8;
    startPos = spacing/2.0;
    barHeight = 0.8f;
    refreshCounter = 0;

	userGainAsChanged = false;
}

GainCanvas::~GainCanvas() {

}

void GainCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
  //  wxPaintDC dc(this);
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

	bool areGainsChangedHere = false;
    
    for (auto gi : gainPanels) {
        if (gi->getChanged()) {
			areGainsChangedHere  = true;
			// Gain only displays integer gain values, so set the applied gain 
			//value to exactly that. 
            wxGetApp().setGain(gi->getName(), (int)(gi->getValue()));
			//A gain may be exposed as setting also so assure refresh of the menu also.
			wxGetApp().notifyMainUIOfDeviceChange(false); //do not rebuild the gain UI

            gi->setChanged(false);
        }
    }

	//User input has changed the gain, so schedule an update of values
	//in 150ms in the future, else the device may not have taken the value into account.
	if (areGainsChangedHere) {
		userGainAsChanged = true;
		userGainAsChangedDelayTimer.start();
	}
	else {
		userGainAsChangedDelayTimer.update();

		if (!userGainAsChanged || (userGainAsChanged && userGainAsChangedDelayTimer.getMilliseconds() > 150)) {
			
			if (updateGainValues()) {
				Refresh();
			}

			userGainAsChanged = false;
		}
	}
}

void GainCanvas::SetLevel() {
    CubicVR::vec2 mpos = mouseTracker.getGLXY();
    
    for (auto gi : gainPanels) {
        if (gi->isMeterHit(mpos)) {
            float value = gi->getMeterHitValue(mpos);
            
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
            float value = gi->getMeterHitValue(mpos);
        
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
    else {
        if (!helpTip.empty()) {
            setStatusText(helpTip);
        }
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

    //possible if we 'Refresh Devices' then devInfo becomes null
    //until a new device is selected.
    if (devInfo == nullptr) {
        return;
    }

    DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(devInfo->getDeviceId());
	
	//read the gains from the device.
	//This may be wrong because the device is not started, or has yet 
	//to take into account a user gain change. Doesn't matter,
	//UpdateGainValues() takes cares of updating the true value realtime.
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

// call this to refresh the gain values only, not the whole UI.
bool GainCanvas::updateGainValues() {

	bool isRefreshNeeded = false;

	SDRDeviceInfo *devInfo = wxGetApp().getDevice();

	//possible if we 'Refresh Devices' then devInfo becomes null
	//until a new device is selected.
	//also, do not attempt an update with the device is not started.
	if (devInfo == nullptr || !devInfo->isActive()) {
		return false;
	}

	DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(devInfo->getDeviceId());

	gains = devInfo->getGains(SOAPY_SDR_RX, 0);
	SDRRangeMap::iterator gi;

	size_t numGainsToRefresh = std::min(gains.size(), gainPanels.size());
	size_t panelIndex = 0;

	//actually the order of gains iteration should be constant because map of string,
	//and gainPanels were built in that order in updateGainUI()
	for (auto gi : gains) {

		if (panelIndex >= numGainsToRefresh) {
			break;
		}

		// do not update if a change is already pending.
		if (!gainPanels[panelIndex]->getChanged()) {

			//read the actual gain from the device, round it
			float actualRoundedGain = (float)std::round(devInfo->getCurrentGain(SOAPY_SDR_RX, 0, gi.first));
			
			//do nothing if the difference is less than 1.0, since the panel do not show it anyway.
			if ((int)actualRoundedGain != (int)(gainPanels[panelIndex]->getValue())) {

				gainPanels[panelIndex]->setValue(actualRoundedGain);

				//update the config with this value : 
				//a consequence of such updates is that the use setting 
				// is overriden by the current one in AGC mode.
				//TODO: if it not desirable, do not update in AGC mode.
				devConfig->setGain(gi.first, actualRoundedGain);

				isRefreshNeeded = true;
			}
		} //end if no external change pending.

		panelIndex++;
	}

	return isRefreshNeeded;
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

