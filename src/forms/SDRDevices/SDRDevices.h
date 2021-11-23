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
    explicit SDRDevicesDialog( wxWindow* parent, const wxPoint &wxPos = wxDefaultPosition);
    
    void OnClose( wxCloseEvent& event ) override;
    void OnDeleteItem( wxTreeEvent& event ) override;
    void OnSelectionChanged( wxTreeEvent& event ) override;
    void OnAddRemote( wxMouseEvent& event ) override;
    void OnUseSelected( wxMouseEvent& event ) override;
    void OnTreeDoubleClick( wxMouseEvent& event ) override;
    void OnDeviceTimer( wxTimerEvent& event ) override;
    void OnRefreshDevices( wxMouseEvent& event ) override;
    void OnPropGridChanged( wxPropertyGridEvent& event ) override;
    void OnPropGridFocus( wxFocusEvent& event ) override;

private:
    void refreshDeviceProperties();
    void doRefreshDevices();
    
    SDRDeviceInfo *getSelectedDevice(wxTreeItemId selId_in);
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