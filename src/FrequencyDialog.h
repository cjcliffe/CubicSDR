#pragma once

#include "wx/dialog.h"
#include "wx/textctrl.h"
#include "wx/string.h"
#include "wx/button.h"


class FrequencyDialog: public wxDialog
{
public:

    FrequencyDialog ( wxWindow * parent, wxWindowID id, const wxString & title,
                  const wxPoint & pos = wxDefaultPosition,
                  const wxSize & size = wxDefaultSize,
                  long style = wxDEFAULT_DIALOG_STYLE );

    wxTextCtrl * dialogText;

private:

    void OnEnter ( wxCommandEvent &event );

    DECLARE_EVENT_TABLE()
};
