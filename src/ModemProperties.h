// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>

#include "DemodulatorInstance.h"
#include "Modem.h"

class ModemProperties : public wxPanel {
public:
    ModemProperties(
        wxWindow *parent,
        wxWindowID winid = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxTAB_TRAVERSAL | wxNO_BORDER,
        const wxString& name = wxPanelNameStr
    );
    ~ModemProperties();
    
    void initDefaultProperties();
    void initProperties(ModemArgInfoList newArgs, DemodulatorInstancePtr demodInstance);
    bool isMouseInView();
    void setCollapsed(bool state);
    bool isCollapsed();
    void fitColumns();
    
    void updateTheme();
    
private:
    wxPGProperty *addArgInfoProperty(wxPropertyGrid *pg, ModemArgInfo arg);
    std::string readProperty(std::string);
    void OnChange(wxPropertyGridEvent &event);
    void OnShow(wxShowEvent &event);
    void OnCollapse(wxPropertyGridEvent &event);
    void OnExpand(wxPropertyGridEvent &event);

    void OnMouseEnter(wxMouseEvent &event);
    void OnMouseLeave(wxMouseEvent &event);
    
    wxBoxSizer* bSizer;
    wxPropertyGrid* m_propertyGrid;
    ModemArgInfoList args;
    DemodulatorInstancePtr demodContext;

    std::map<std::string, wxPGProperty *> props;
    bool mouseInView, collapsed;
    
    ModemArgInfoList defaultArgs;
    ModemArgInfo outputArg;
    std::map<std::string, wxPGProperty *> defaultProps;
    
    std::vector<RtAudio::DeviceInfo> audioDevices;
    std::map<int,RtAudio::DeviceInfo> audioInputDevices;
    std::map<int,RtAudio::DeviceInfo> audioOutputDevices;
};