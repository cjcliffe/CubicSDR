#include "PortSelectorDialogBase.h"

class PortSelectorDialog : public PortSelectorDialogBase {
public:
    PortSelectorDialog( wxWindow* parent, wxWindowID id = wxID_ANY, std::string defaultPort = "" );

protected:
    void onListSelect( wxCommandEvent& event );
    void onCancelButton( wxCommandEvent& event );
    void onOKButton( wxCommandEvent& event );
};
