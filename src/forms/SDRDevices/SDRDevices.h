// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <map>
#include <vector>

#include "SDRDevicesForm.h"
#include "SoapySDRThread.h"
#include "SDREnumerator.h"
#include "SDRDeviceAdd.h"

class SDRDevicesDialog: public devFrame {
public:
    SDRDevicesDialog( wxWindow* parent );
    
    void OnClose( wxCloseEvent& event );
    void OnDeleteItem( wxTreeEvent& event );
    void OnSelectionChanged( wxTreeEvent& event );
    void OnAddRemote( wxMouseEvent& event );
    void OnUseSelected( wxMouseEvent& event );
    void OnTreeDoubleClick( wxMouseEvent& event );
    void OnDeviceTimer( wxTimerEvent& event );
    void OnRefreshDevices( wxMouseEvent& event );
    void OnPropGridChanged( wxPropertyGridEvent& event );
    void OnPropGridFocus( wxFocusEvent& event );

private:
    void refreshDeviceProperties();
    void doRefreshDevices();
    
    SDRDeviceInfo *getSelectedDevice(wxTreeItemId selId);
    wxPGProperty *addArgInfoProperty(wxPropertyGrid *pg, SoapySDR::ArgInfo arg);

    //
    std::string getSelectedChoiceOption(wxPGProperty* prop, const SoapySDR::ArgInfo& arg);


    bool refresh, failed;
    std::map<std::string, std::vector<SDRDeviceInfo *>* > devs;
    std::vector<SDRDeviceInfo *>::iterator devs_i;
    std::map<wxTreeItemId, SDRDeviceInfo *> devItems;
    std::map<wxTreeItemId, SDRDeviceInfo *>::iterator devItems_i;
    SDRDeviceInfo *dev;
    std::map<std::string, SoapySDR::ArgInfo> deviceArgs;
    std::map<std::string, wxPGProperty *> runtimeProps;
    std::map<std::string, SoapySDR::ArgInfo> runtimeArgs;
    std::map<std::string, wxPGProperty *> streamProps;
    std::map<std::string, wxPGProperty *> devSettings;
    wxTreeItemId selId;
    wxTreeItemId editId;
    wxTreeItemId removeId;
    SDRDeviceAddDialog *devAddDialog;
};