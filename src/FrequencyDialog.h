#pragma once

#include "wx/dialog.h"
#include "wx/textctrl.h"
#include "wx/string.h"
#include "wx/button.h"

#define wxID_FREQ_INPUT 3001

class FrequencyDialog: public wxDialog
{
public:

    FrequencyDialog ( wxWindow * parent, wxWindowID id, const wxString & title,
                  const wxPoint & pos = wxDefaultPosition,
                  const wxSize & size = wxDefaultSize,
                  long style = wxDEFAULT_DIALOG_STYLE );

    wxTextCtrl * dialogText;

    long long strToFrequency(std::string freqStr);
    std::string frequencyToStr(long long freq);

private:

    void OnEnter ( wxCommandEvent &event );
    void OnChar ( wxKeyEvent &event );
    std::string& filterChars(std::string& s, const std::string& allowed);
    DECLARE_EVENT_TABLE()
};
