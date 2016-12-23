#include "ActionDialog.h"


ActionDialog *ActionDialog::activeDialog = nullptr;

ActionDialog::ActionDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : ActionDialogBase(parent, id, title, pos, size, style) {
}


ActionDialog::~ActionDialog() {

}

void ActionDialog::showDialog(ActionDialog *dlg) {
    if (activeDialog) { // rejected
        delete dlg;
        return;
    }
    activeDialog = dlg;
    dlg->Layout();
    dlg->Fit();
    dlg->ShowModal();
}

ActionDialog *ActionDialog::getActiveDialog() {
    return activeDialog;
}


void ActionDialog::setActiveDialog(ActionDialog *dlg) {
    activeDialog = dlg;
}


void ActionDialog::onClickCancel( wxCommandEvent& event ) {
    doClickCancel();
    activeDialog->EndModal(0);
    ActionDialog::setActiveDialog(nullptr);
    delete activeDialog;
}


void ActionDialog::onClickOK( wxCommandEvent& event ) {
    doClickOK();
    activeDialog->EndModal(0);
    ActionDialog::setActiveDialog(nullptr);
    delete activeDialog;
}


void ActionDialog::doClickCancel() {
    
}


void ActionDialog::doClickOK() {
    
}
