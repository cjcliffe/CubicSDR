#include "PortSelectorDialog.h"

#include "rs232.h"
#include "CubicSDR.h"

PortSelectorDialog::PortSelectorDialog( wxWindow* parent, wxWindowID id, std::string defaultPort ) : PortSelectorDialogBase(parent, id) {
    comEnumerate();
    
    int nPorts = comGetNoPorts();
    
    for (int i = 0; i < nPorts; i++) {
#ifdef WIN32
        m_portList->Append(comGetPortName(i));
#else
        m_portList->Append(comGetInternalName(i));
#endif
    }
    
    comTerminate();
    
    m_portSelection->SetValue(defaultPort);
}

void PortSelectorDialog::onListSelect( wxCommandEvent& /* event */ ) {
    int pSelect = m_portList->GetSelection();
    if (pSelect != -1) {
        m_portSelection->SetValue(m_portList->GetString(pSelect));
    }
}


void PortSelectorDialog::onCancelButton( wxCommandEvent& /* event */ ) {
    wxGetApp().getAppFrame()->dismissRigControlPortDialog();
}


void PortSelectorDialog::onOKButton( wxCommandEvent& /* event */ ) {
    wxGetApp().getAppFrame()->setRigControlPort(m_portSelection->GetValue().ToStdString());
}
