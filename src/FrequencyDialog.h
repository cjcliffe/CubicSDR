// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/dialog.h"
#include "wx/textctrl.h"
#include "wx/string.h"
#include "wx/button.h"
#include "DemodulatorInstance.h"

#define wxID_FREQ_INPUT 3001

class FrequencyDialog: public wxDialog
{
public:
    typedef enum FrequencyDialogTarget {
        FDIALOG_TARGET_DEFAULT,
        FDIALOG_TARGET_CENTERFREQ,
        FDIALOG_TARGET_FREQ,
        FDIALOG_TARGET_BANDWIDTH,
        FDIALOG_TARGET_WATERFALL_LPS,
        FDIALOG_TARGET_SPECTRUM_AVG,
        FDIALOG_TARGET_GAIN
    } FrequencyDialogTarget;
    FrequencyDialog ( wxWindow * parent, wxWindowID id, const wxString & title,
                  DemodulatorInstancePtr demod = nullptr,
                  const wxPoint & pos = wxDefaultPosition,
                  const wxSize & size = wxDefaultSize,
                  long style = wxDEFAULT_DIALOG_STYLE,
                  FrequencyDialogTarget targetMode = FDIALOG_TARGET_DEFAULT,
                  wxString initString = "");

    wxTextCtrl * dialogText;

private:
    DemodulatorInstancePtr activeDemod;
    void OnChar ( wxKeyEvent &event );
	void OnShow(wxShowEvent &event);
    FrequencyDialogTarget targetMode;
	std::string initialString;
    DECLARE_EVENT_TABLE()
};
