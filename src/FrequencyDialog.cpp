#include "FrequencyDialog.h"

#include "wx/clipbrd.h"
#include <sstream>
#include <iomanip>
#include "CubicSDR.h"

wxBEGIN_EVENT_TABLE(FrequencyDialog, wxDialog)
EVT_CHAR_HOOK(FrequencyDialog::OnChar)
wxEND_EVENT_TABLE()

FrequencyDialog::FrequencyDialog(wxWindow * parent, wxWindowID id, const wxString & title, DemodulatorInstance *demod, const wxPoint & position,
        const wxSize & size, long style) :
        wxDialog(parent, id, title, position, size, style) {
    wxString freqStr;
    activeDemod = demod;

    if (activeDemod) {
        freqStr = frequencyToStr(activeDemod->getFrequency());
    } else {
        freqStr = frequencyToStr(wxGetApp().getFrequency());
    }

    dialogText = new wxTextCtrl(this, wxID_FREQ_INPUT, freqStr, wxPoint(6, 1), wxSize(size.GetWidth() - 20, size.GetHeight() - 70),
    wxTE_PROCESS_ENTER);
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

std::string FrequencyDialog::frequencyToStr(long long freq) {
    long double freqTemp;

    freqTemp = freq;
    std::string suffix("");
    std::stringstream freqStr;

    if (freqTemp >= 1.0e9) {
        freqTemp /= 1.0e9;
        freqStr << std::setprecision(10);
        suffix = std::string("GHz");
    } else if (freqTemp >= 1.0e6) {
        freqTemp /= 1.0e6;
        freqStr << std::setprecision(7);
        suffix = std::string("MHz");
    } else if (freqTemp >= 1.0e3) {
        freqTemp /= 1.0e3;
        freqStr << std::setprecision(4);
        suffix = std::string("KHz");
    }

    freqStr << freqTemp;
    freqStr << suffix;

    return freqStr.str();
}

long long FrequencyDialog::strToFrequency(std::string freqStr) {
    std::string filterStr = filterChars(freqStr, std::string("0123456789.MKGHmkgh"));

    int numLen = filterStr.find_first_not_of("0123456789.");

    if (numLen == std::string::npos) {
        numLen = freqStr.length();
    }

    std::string numPartStr = freqStr.substr(0, numLen);
    std::string suffixStr = freqStr.substr(numLen);

    std::stringstream numPartStream;
    numPartStream.str(numPartStr);

    long double freqTemp = 0;

    numPartStream >> freqTemp;

    if (suffixStr.length()) {
        if (suffixStr.find_first_of("Gg") != std::string::npos) {
            freqTemp *= 1.0e9;
        } else if (suffixStr.find_first_of("Mm") != std::string::npos) {
            freqTemp *= 1.0e6;
        } else if (suffixStr.find_first_of("Kk") != std::string::npos) {
            freqTemp *= 1.0e3;
        } else if (suffixStr.find_first_of("Hh") != std::string::npos) {
            // ...
        }
    } else if (numPartStr.find_first_of(".") != std::string::npos || freqTemp <= 3000) {
        freqTemp *= 1.0e6;
    }

    return (long long) freqTemp;
}

void FrequencyDialog::OnChar(wxKeyEvent& event) {
    int c = event.GetKeyCode();
    long long freq;

    switch (c) {
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        // Do Stuff
        freq = strToFrequency(dialogText->GetValue().ToStdString());
        if (activeDemod) {
            activeDemod->setTracking(true);
            activeDemod->setFollow(true);
            activeDemod->setFrequency(freq);
            activeDemod->updateLabel(freq);
        } else {
            wxGetApp().setFrequency(freq);
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
