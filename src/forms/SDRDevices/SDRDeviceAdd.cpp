#include "SDRDeviceAdd.h"

#include "SDREnumerator.h"

SDRDeviceAddDialog::SDRDeviceAddDialog( wxWindow* parent ): SDRDeviceAddForm( parent ) {
    okPressed = false;
    selectedModule = "";
    moduleParam = "";
    selectedModule = "SoapyRemote";
    
    m_soapyModule->Append("SoapyRemote");
    m_paramLabel->SetLabel("Remote Address (address[:port])");
    
    std::vector<std::string> &factories = SDREnumerator::getFactories();
    std::vector<std::string>::iterator factory_i;
    
    for (factory_i = factories.begin(); factory_i != factories.end(); factory_i++) {
        if (*factory_i != "remote" && *factory_i != "null") {
            m_soapyModule->Append(*factory_i);
        }
    }
}

void SDRDeviceAddDialog::OnSoapyModuleChanged( wxCommandEvent& /* event */) {
    wxString strSel = m_soapyModule->GetStringSelection();
    
    selectedModule = strSel.ToStdString();
    
    if (selectedModule == "SoapyRemote") {
        m_paramLabel->SetLabelText("Remote Address (address[:port])");
    } else {
        m_paramLabel->SetLabel("SoapySDR Device Parameters, i.e. 'addr=192.168.1.105'");
    }
}

void SDRDeviceAddDialog::OnCancelButton( wxCommandEvent& /* event */) {
    okPressed = false;
    Close(true);
}

void SDRDeviceAddDialog::OnOkButton( wxCommandEvent& /* event */) {
    wxString strSel = m_soapyModule->GetStringSelection();
    selectedModule = strSel.ToStdString();
    moduleParam = m_paramText->GetValue().ToStdString();
    okPressed = true;
    Close(true);
}

bool SDRDeviceAddDialog::wasOkPressed() {
    return okPressed;
}

std::string SDRDeviceAddDialog::getSelectedModule() {
    return selectedModule;
}

std::string SDRDeviceAddDialog::getModuleParam() {
    return moduleParam;
}
