#include "PortSelectorDialogBase.h"

class PortSelectorDialog : public PortSelectorDialogBase {
public:
    explicit PortSelectorDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const std::string& defaultPort = "" );

protected:
    void onListSelect( wxCommandEvent& event ) override;
    void onCancelButton( wxCommandEvent& event ) override;
    void onOKButton( wxCommandEvent& event ) override;
    void onClose( wxCloseEvent& event ) override;
};
