// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "SDRDeviceAddForm.h"

class SDRDeviceAddDialog : public SDRDeviceAddForm {
public:
    explicit SDRDeviceAddDialog( wxWindow* parent );

    void OnSoapyModuleChanged( wxCommandEvent& event ) override;
    void OnCancelButton( wxCommandEvent& event ) override;
    void OnOkButton( wxCommandEvent& event ) override;
    
    bool wasOkPressed() const;
    std::string getSelectedModule();
    std::string getModuleParam();
    
private:
    bool okPressed;
    std::string selectedModule;
    std::string moduleParam;
};