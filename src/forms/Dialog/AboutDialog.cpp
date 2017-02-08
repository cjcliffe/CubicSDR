// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AboutDialog.h"

AboutDialog::AboutDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
: AboutDialogBase(parent, id, title, pos, size, style) {
    m_appName->SetLabelText(CUBICSDR_INSTALL_NAME " v" CUBICSDR_VERSION);
}
