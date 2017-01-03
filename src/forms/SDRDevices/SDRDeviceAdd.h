// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "SDRDeviceAddForm.h"

class SDRDeviceAddDialog : public SDRDeviceAddForm {
public:
    SDRDeviceAddDialog( wxWindow* parent );

    void OnSoapyModuleChanged( wxCommandEvent& event );
    void OnCancelButton( wxCommandEvent& event );
    void OnOkButton( wxCommandEvent& event );
    
    bool wasOkPressed();
    std::string getSelectedModule();
    std::string getModuleParam();
    
private:
    bool okPressed;
    std::string selectedModule;
    std::string moduleParam;
};