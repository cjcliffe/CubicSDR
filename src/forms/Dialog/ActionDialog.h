#include "ActionDialogBase.h"
#include <mutex>

class ActionDialog : public ActionDialogBase {
public:
    ActionDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("QuestionTitle"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
    ~ActionDialog();

    void onClickCancel( wxCommandEvent& event );
    void onClickOK( wxCommandEvent& event );

    virtual void doClickCancel();
    virtual void doClickOK();
    
    static ActionDialog *getActiveDialog();
    static void setActiveDialog(ActionDialog *dlg);
    static void showDialog(ActionDialog *dlg);

private:
    static ActionDialog *activeDialog;
};
