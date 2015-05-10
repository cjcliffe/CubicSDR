#include "FrequencyDialog.h"

#include "wx/clipbrd.h"

wxBEGIN_EVENT_TABLE(FrequencyDialog, wxDialog)
EVT_CHAR_HOOK(FrequencyDialog::OnChar)
wxEND_EVENT_TABLE()

FrequencyDialog::FrequencyDialog(wxWindow * parent, wxWindowID id, const wxString & title, const wxPoint & position, const wxSize & size, long style) :
wxDialog(parent, id, title, position, size, style) {
    wxString freqStr = "105.7Mhz";

    dialogText = new wxTextCtrl(this, wxID_FREQ_INPUT, freqStr, wxPoint(6, 1), wxSize(size.GetWidth() - 20, size.GetHeight() - 70), wxTE_PROCESS_ENTER);
    dialogText->SetFont(wxFont(20, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    Centre();

    dialogText->SetSelection(-1, -1);
}

std::string& FrequencyDialog::filterChars(std::string& s, const std::string& allowed) {
    s.erase(remove_if(s.begin(), s.end(), [&allowed](const char& c) {
        return allowed.find(c) == std::string::npos;
    }), s.end());
    return s;
}

void FrequencyDialog::OnChar(wxKeyEvent& event) {
    wxChar c = event.GetKeyCode();

    switch (c) {
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        // Do Stuff

        Close();
        break;
    case WXK_ESCAPE:
        Close();
        break;
    }

    std::string allowed("0123456789.MKGHZmkghz");

    if (allowed.find_first_of(c) != std::string::npos || c == WXK_BACK) {
        event.DoAllowNextEvent();
    } else if (event.ControlDown() && c == 'V') {
        // Alter clipboard contents to remove unwanted chars
        wxTheClipboard->Open();
        wxTextDataObject data;
        wxTheClipboard->GetData(data);
        std::string clipText = data.GetText().ToStdString();
        std::string pasteText = filterChars(clipText,std::string(allowed));
        wxTheClipboard->SetData(new wxTextDataObject(pasteText));
        wxTheClipboard->Close();
        event.Skip();
    } else if (c == WXK_RIGHT || c == WXK_LEFT || event.ControlDown()) {
        event.Skip();
    } else {
        std::cout << (int) c << std::endl;
    }
}
