// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "ActionDialog.h"


ActionDialog *ActionDialog::activeDialog = nullptr;

ActionDialog::ActionDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : ActionDialogBase(parent, id, title, pos, size, style) {
}


ActionDialog::~ActionDialog() = default;

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


void ActionDialog::onClickCancel( wxCommandEvent& /* event */ ) {
    ActionDialog *dlg = activeDialog;
    ActionDialog::setActiveDialog(nullptr);
    dlg->EndModal(0);
    doClickCancel();
    delete dlg;
}


void ActionDialog::onClickOK( wxCommandEvent& /* event */ ) {
    ActionDialog *dlg = activeDialog;
    ActionDialog::setActiveDialog(nullptr);
    dlg->EndModal(0);
    doClickOK();
    delete dlg;
}


void ActionDialog::doClickCancel() {
    
}


void ActionDialog::doClickOK() {
    
}
