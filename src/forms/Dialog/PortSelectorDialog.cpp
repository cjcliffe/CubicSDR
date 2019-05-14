#include "PortSelectorDialog.h"

#include "rs232.h"
#include "CubicSDR.h"

PortSelectorDialog::PortSelectorDialog( wxWindow* parent, wxWindowID id, std::string defaultPort ) : PortSelectorDialogBase(parent, id) {
    comEnumerate();
    
    int nPorts = comGetNoPorts();

    if (!defaultPort.empty()) {
        m_portList->Append(defaultPort);
    }

    for (int i = 0; i < nPorts; i++) {
#ifdef WIN32
        string portName(comGetPortName(i));
#else
        string portName(comGetInternalName(i));
#endif
        if (portName != defaultPort) {
            m_portList->Append(portName);
        }
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


void PortSelectorDialog::onClose(wxCloseEvent & /* event */) {
    wxGetApp().getAppFrame()->dismissRigControlPortDialog();
}
