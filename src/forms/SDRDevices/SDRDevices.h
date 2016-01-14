#pragma once

#include <map>
#include <vector>

#include "SDRDevicesForm.h"
#include "SoapySDRThread.h"
#include "SDREnumerator.h"

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

private:
    SDRDeviceInfo *getSelectedDevice(wxTreeItemId selId);
    wxPGProperty *addArgInfoProperty(wxPropertyGrid *pg, SoapySDR::ArgInfo arg);

    bool refresh, failed;
    std::map<std::string, std::vector<SDRDeviceInfo *>* > devs;
    std::vector<SDRDeviceInfo *>::iterator devs_i;
    std::map<wxTreeItemId, SDRDeviceInfo *> devItems;
    std::map<wxTreeItemId, SDRDeviceInfo *>::iterator devItems_i;
    SDRDeviceInfo *dev = NULL;
    std::vector<wxPGProperty *> props;
};