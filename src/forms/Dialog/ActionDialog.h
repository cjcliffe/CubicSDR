// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ActionDialogBase.h"
#include <mutex>

class ActionDialog : public ActionDialogBase {
public:
    explicit ActionDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("QuestionTitle"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
    ~ActionDialog() override;

    void onClickCancel( wxCommandEvent& event ) override;
    void onClickOK( wxCommandEvent& event ) override;

    virtual void doClickCancel();
    virtual void doClickOK();
    
    static ActionDialog *getActiveDialog();
    static void setActiveDialog(ActionDialog *dlg);
    static void showDialog(ActionDialog *dlg);

private:
    static ActionDialog *activeDialog;
};
