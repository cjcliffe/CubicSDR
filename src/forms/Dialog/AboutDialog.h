// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "AboutDialogBase.h"
#include "CubicSDRDefs.h"

class AboutDialog : public AboutDialogBase {
public:
  		AboutDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("About"),
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 530, 420 ),
                    long style = wxDEFAULT_DIALOG_STYLE );
  
};
