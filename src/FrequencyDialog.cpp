#include "FrequencyDialog.h"

#include "wx/clipbrd.h"
#include <sstream>
#include "CubicSDR.h"

wxBEGIN_EVENT_TABLE(FrequencyDialog, wxDialog)
EVT_CHAR_HOOK(FrequencyDialog::OnChar)
wxEND_EVENT_TABLE()

FrequencyDialog::FrequencyDialog(wxWindow * parent, wxWindowID id, const wxString & title, DemodulatorInstance *demod, const wxPoint & position,
        const wxSize & size, long style, FrequencyDialogTarget targetMode) :
        wxDialog(parent, id, title, position, size, style) {
    wxString freqStr;
    activeDemod = demod;
    this->targetMode = targetMode;

    if (targetMode == FDIALOG_TARGET_DEFAULT) {
        if (activeDemod) {
            freqStr = frequencyToStr(activeDemod->getFrequency());
        } else {
            freqStr = frequencyToStr(wxGetApp().getFrequency());
        }
    }
            
    if (targetMode == FDIALOG_TARGET_BANDWIDTH) {
        freqStr = frequencyToStr(wxGetApp().getDemodMgr().getLastBandwidth());
    }

    dialogText = new wxTextCtrl(this, wxID_FREQ_INPUT, freqStr, wxPoint(6, 1), wxSize(size.GetWidth() - 20, size.GetHeight() - 70),
    wxTE_PROCESS_ENTER);
    dialogText->SetFont(wxFont(20, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    Centre();

    dialogText->SetSelection(-1, -1);
}


void FrequencyDialog::OnChar(wxKeyEvent& event) {
    int c = event.GetKeyCode();
    long long freq;

    switch (c) {
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        // Do Stuff
        freq = strToFrequency(dialogText->GetValue().ToStdString());
            if (targetMode == FDIALOG_TARGET_DEFAULT) {
                if (activeDemod) {
                    activeDemod->setTracking(true);
                    activeDemod->setFollow(true);
                    activeDemod->setFrequency(freq);
                    activeDemod->updateLabel(freq);
                } else {
                    wxGetApp().setFrequency(freq);
                }
            }
            if (targetMode == FDIALOG_TARGET_BANDWIDTH) {
                if (activeDemod) {
                    activeDemod->setBandwidth(freq);
                } else {
                    wxGetApp().getDemodMgr().setLastBandwidth(freq);
                }
            }
        Close();
        break;
    case WXK_ESCAPE:
        Close();
        break;
    }

    std::string allowed("0123456789.MKGHZmkghz");

    if (allowed.find_first_of(c) != std::string::npos || c == WXK_DELETE || c == WXK_BACK || c == WXK_NUMPAD_DECIMAL
            || (c >= WXK_NUMPAD0 && c <= WXK_NUMPAD9)) {
#ifdef __linux__
        dialogText->OnChar(event);
        event.Skip();
#else
        event.DoAllowNextEvent();
#endif
    } else if (event.ControlDown() && c == 'V') {
        // Alter clipboard contents to remove unwanted chars
        wxTheClipboard->Open();
        wxTextDataObject data;
        wxTheClipboard->GetData(data);
        std::string clipText = data.GetText().ToStdString();
        std::string pasteText = filterChars(clipText, std::string(allowed));
        wxTheClipboard->SetData(new wxTextDataObject(pasteText));
        wxTheClipboard->Close();
        event.Skip();
    } else if (c == WXK_RIGHT || c == WXK_LEFT || event.ControlDown()) {
        event.Skip();
    }
}
