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
    typedef enum FrequencyDialogTarget { FDIALOG_TARGET_DEFAULT, FDIALOG_TARGET_CENTERFREQ, FDIALOG_TARGET_FREQ, FDIALOG_TARGET_BANDWIDTH } FrequencyDialogTarget;
    FrequencyDialog ( wxWindow * parent, wxWindowID id, const wxString & title,
                  DemodulatorInstance *demod = NULL,
                  const wxPoint & pos = wxDefaultPosition,
                  const wxSize & size = wxDefaultSize,
                  long style = wxDEFAULT_DIALOG_STYLE,
                  FrequencyDialogTarget targetMode = FDIALOG_TARGET_DEFAULT);

    wxTextCtrl * dialogText;

private:
    DemodulatorInstance *activeDemod;
    void OnEnter ( wxCommandEvent &event );
    void OnChar ( wxKeyEvent &event );
    FrequencyDialogTarget targetMode;
    DECLARE_EVENT_TABLE()
};
