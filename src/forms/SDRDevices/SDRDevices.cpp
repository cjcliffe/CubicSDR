#include "SDRDevices.h"
#include "CubicSDR.h"

SDRDevicesDialog::SDRDevicesDialog( wxWindow* parent ): devFrame( parent ) {
    wxTreeItemId devRoot = devTree->AddRoot("Devices");
    wxTreeItemId localBranch = devTree->AppendItem(devRoot, "Local");
    wxTreeItemId remoteBranch = devTree->AppendItem(devRoot, "Remote");
    
    devs = SDREnumerator::enumerate_devices();
    for (devs_i = devs->begin(); devs_i != devs->end(); devs_i++) {
        devItems[devTree->AppendItem(localBranch, (*devs_i)->getName())] = (*devs_i);
    }
    
    devTree->ExpandAll();
}


void SDRDevicesDialog::OnDeleteItem( wxTreeEvent& event ) {
    event.Skip();
}

void SDRDevicesDialog::OnSelectionChanged( wxTreeEvent& event ) {
    event.Skip();
}

void SDRDevicesDialog::OnAddRemote( wxMouseEvent& event ) {
    event.Skip();
}

void SDRDevicesDialog::OnUseSelected( wxMouseEvent& event ) {
    wxTreeItemId selId = devTree->GetSelection();
    
    devItems_i = devItems.find(selId);
    if (devItems_i != devItems.end()) {
        dev = devItems[selId];
        wxGetApp().setDevice(dev);
        Close();
    }
}

