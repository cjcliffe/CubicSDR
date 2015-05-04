#include "FrequencyDialog.h"

wxBEGIN_EVENT_TABLE(FrequencyDialog, wxDialog) wxEND_EVENT_TABLE()

FrequencyDialog::FrequencyDialog(wxWindow * parent, wxWindowID id, const wxString & title, const wxPoint & position, const wxSize & size, long style) :
        wxDialog(parent, id, title, position, size, style) {
    wxString freqStr = "105.7Mhz";

    dialogText = new wxTextCtrl(this, -1, freqStr, wxPoint(6, 1), wxSize(size.GetWidth() - 20, size.GetHeight() - 70), wxTE_PROCESS_ENTER);
    dialogText->SetFont(wxFont(20, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(FrequencyDialog::OnEnter));

    SetEscapeId(wxID_CANCEL);

    Centre();
}

void FrequencyDialog::OnEnter(wxCommandEvent &event) {
    std::cout << dialogText->GetValue().ToStdString() << std::endl;
    Close();
}
