#include "FrequencyDialog.h"

#include "wx/clipbrd.h"
#include <sstream>
#include "CubicSDR.h"

wxBEGIN_EVENT_TABLE(FrequencyDialog, wxDialog)
EVT_CHAR_HOOK(FrequencyDialog::OnChar)
wxEND_EVENT_TABLE()

FrequencyDialog::FrequencyDialog(wxWindow * parent, wxWindowID id, const wxString & title, DemodulatorInstance *demod, const wxPoint & position,
        const wxSize & size, long style, FrequencyDialogTarget targetMode, wxString initString) :
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
        std::string lastDemodType = activeDemod?activeDemod->getDemodulatorType():wxGetApp().getDemodMgr().getLastDemodulatorType();
        if (lastDemodType == "USB" || lastDemodType == "LSB") {
            freqStr = frequencyToStr(wxGetApp().getDemodMgr().getLastBandwidth()/2);
        } else {
            freqStr = frequencyToStr(wxGetApp().getDemodMgr().getLastBandwidth());
        }
    }
            
    if (targetMode == FDIALOG_TARGET_WATERFALL_LPS) {
        freqStr = std::to_string(wxGetApp().getAppFrame()->getWaterfallDataThread()->getLinesPerSecond());
    }

    if (targetMode == FDIALOG_TARGET_SPECTRUM_AVG) {
        freqStr = std::to_string(wxGetApp().getSpectrumProcessor()->getFFTAverageRate());
    }
            
    dialogText = new wxTextCtrl(this, wxID_FREQ_INPUT, freqStr, wxPoint(6, 1), wxSize(size.GetWidth() - 20, size.GetHeight() - 70),
    wxTE_PROCESS_ENTER);
    dialogText->SetFont(wxFont(20, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    Centre();

    if (initString != "" && initString.length() == 1) {
        dialogText->SetValue(initString);
        dialogText->SetSelection(initString.length(), initString.length());
        dialogText->SetFocus();
    } else {
        if (initString != "") {
            dialogText->SetValue(initString);
        }
        dialogText->SetSelection(-1, -1);
    }
}


void FrequencyDialog::OnChar(wxKeyEvent& event) {
    int c = event.GetKeyCode();
    long long freq;
    double dblval;
    std::string lastDemodType = activeDemod?activeDemod->getDemodulatorType():wxGetApp().getDemodMgr().getLastDemodulatorType();
    std::string strValue = dialogText->GetValue().ToStdString();
    
    switch (c) {
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        // Do Stuff

        if (targetMode == FDIALOG_TARGET_DEFAULT) {
            freq = strToFrequency(strValue);
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
            freq = strToFrequency(strValue);
            if (lastDemodType == "USB" || lastDemodType == "LSB") {
                freq *= 2;
            }
            if (activeDemod) {
                activeDemod->setBandwidth(freq);
            } else {
                wxGetApp().getDemodMgr().setLastBandwidth(freq);
            }
        }
        if (targetMode == FDIALOG_TARGET_WATERFALL_LPS) {
            try {
                freq = std::stoi(strValue);
            } catch (exception e) {
                Close();
                break;
            }
            if (freq > 1024) {
                freq = 1024;
            }
            if (freq < 1) {
                freq = 1;
            }
            wxGetApp().getAppFrame()->setWaterfallLinesPerSecond(freq);
        }
        if (targetMode == FDIALOG_TARGET_SPECTRUM_AVG) {
            try {
                dblval = std::stod(strValue);
            } catch (exception e) {
                Close();
                break;
            }
            if (dblval > 0.99) {
                dblval = 0.99;
            }
            if (dblval < 0.1) {
                dblval = 0.1;
            }
            wxGetApp().getAppFrame()->setSpectrumAvgSpeed(dblval);
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
