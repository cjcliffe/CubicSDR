#pragma once


#include "SDRDevices.h"
#include "SDRDevicesForm.h"
#include "SoapySDRThread.h"
#include "SDREnumerator.h"

class SDRDevicesDialog: public devFrame {
public:
    SDRDevicesDialog( wxWindow* parent );
    
    void OnDeleteItem( wxTreeEvent& event );
    void OnSelectionChanged( wxTreeEvent& event );
    void OnAddRemote( wxMouseEvent& event );
    void OnUseSelected( wxMouseEvent& event );
    
private:
    std::vector<SDRDeviceInfo *> *devs;
    std::vector<SDRDeviceInfo *>::iterator devs_i;
    std::map<wxTreeItemId, SDRDeviceInfo *> devItems;
    std::map<wxTreeItemId, SDRDeviceInfo *>::iterator devItems_i;
    SDRDeviceInfo *dev = NULL;
};