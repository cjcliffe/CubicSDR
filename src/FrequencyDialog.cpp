// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "FrequencyDialog.h"

#include "wx/clipbrd.h"
#include <sstream>
#include "CubicSDR.h"

wxBEGIN_EVENT_TABLE(FrequencyDialog, wxDialog)
EVT_CHAR_HOOK(FrequencyDialog::OnChar)
EVT_SHOW(FrequencyDialog::OnShow)
wxEND_EVENT_TABLE()

FrequencyDialog::FrequencyDialog(wxWindow * parent, wxWindowID id, const wxString & title, DemodulatorInstancePtr demod, const wxPoint & position,
        const wxSize & size, long style, FrequencyDialogTarget targetMode, wxString initString) :
        wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, style) {
    wxString freqStr;
    wxSizer *dialogsizer = new wxBoxSizer( wxVERTICAL );
    int titleX, textCtrlX;

    activeDemod = demod;

    this->targetMode = targetMode;
    this->initialString = initString;

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

    if (targetMode == FDIALOG_TARGET_GAIN) {
        if (wxGetApp().getActiveGainEntry() != "") {
            freqStr = std::to_string((int)wxGetApp().getGain(wxGetApp().getActiveGainEntry()));
        }
    }
    dialogText = new wxTextCtrl(this, wxID_FREQ_INPUT, freqStr, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    dialogText->SetFont(wxFont(15, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    if (initString != "" && initString.length() == 1) {
        dialogText->SetValue(initString);
        dialogText->SetSelection(2, 2);
        dialogText->SetFocus();
    } else {
        if (initString != "") {
            dialogText->SetValue(initString);
        }
        dialogText->SetSelection(-1, -1);
    }

    // Set the textControl width to the [title + 35%] or the [content +10%],
    // whichever's the greater.

    textCtrlX = dialogText->GetTextExtent(initString).GetWidth();
    titleX = this->GetTextExtent(title).GetWidth();
    dialogText->SetMinSize(wxSize(max(int(1.35 * titleX), int(1.1 * textCtrlX)), -1));
    dialogsizer->Add( dialogText, wxSizerFlags(1).Expand().Border(wxALL, 5));
    SetSizerAndFit(dialogsizer);

}


void FrequencyDialog::OnChar(wxKeyEvent& event) {
    int c = event.GetKeyCode();
    long long freq, freq2, freq_ctr, range_bw;
    double dblval;
    std::string lastDemodType = activeDemod?activeDemod->getDemodulatorType():wxGetApp().getDemodMgr().getLastDemodulatorType();
    std::string strValue = dialogText->GetValue().ToStdString();
    bool ranged = false;
    std::string strValue2;
    size_t range_pos;


    switch (c) {
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        // Do Stuff
        ranged = false;
        if ((range_pos = strValue.find_first_of("-")) > 0) {
            strValue2 = strValue.substr(range_pos+1);
            strValue = strValue.substr(0,range_pos);

            if (targetMode == FDIALOG_TARGET_DEFAULT && !activeDemod && strValue.length() && strValue2.length()) {
                ranged = true;
            }
        }

        if (targetMode == FDIALOG_TARGET_DEFAULT || targetMode == FDIALOG_TARGET_FREQ) {
            if (ranged) {
                freq = strToFrequency(strValue);
                freq2 = strToFrequency(strValue2);
            } else {
                freq = strToFrequency(strValue);
            }
            if (activeDemod) {
                activeDemod->setFrequency(freq);
                activeDemod->updateLabel(freq);

                freq_ctr = wxGetApp().getFrequency();
                range_bw = wxGetApp().getSampleRate();

                if (freq_ctr - (range_bw / 2) > freq || freq_ctr + (range_bw / 2) < freq) {
                    wxGetApp().setFrequency(freq);
                }

            } else {
                if (ranged && (freq || freq2)) {
                    if (freq > freq2) {
                        std::swap(freq,freq2);
                    }
                    range_bw = (freq2-freq);
                    freq_ctr = freq + (range_bw/2);
                    if (range_bw > wxGetApp().getSampleRate()) {
                        range_bw = wxGetApp().getSampleRate();
                    }
                    if (range_bw < 30000) {
                        range_bw = 30000;
                    }
                    if (freq == freq2) {
                        wxGetApp().setFrequency(freq_ctr);
                        wxGetApp().getAppFrame()->setViewState();
                    } else {
                        if (wxGetApp().getSampleRate()/4 > range_bw) {
                            wxGetApp().setFrequency(freq_ctr + wxGetApp().getSampleRate()/4);
                        } else {
                            wxGetApp().setFrequency(freq_ctr);
                        }
                        wxGetApp().getAppFrame()->setViewState(freq_ctr, range_bw);
                    }
                } else {
                    wxGetApp().setFrequency(freq);
                }
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

        if (targetMode == FDIALOG_TARGET_GAIN) {
            try {
                freq = std::stoi(strValue);
            } catch (exception e) {
                break;
            }
            SDRDeviceInfo *devInfo = wxGetApp().getDevice();
            std::string gainName = wxGetApp().getActiveGainEntry();
            if (gainName == "") {
                break;
            }
            SDRRangeMap gains = devInfo->getGains(SOAPY_SDR_RX, 0);
            if (freq > gains[gainName].maximum()) {
                freq = gains[gainName].maximum();
            }
            if (freq < gains[gainName].minimum()) {
                freq = gains[gainName].minimum();
            }
            wxGetApp().setGain(gainName, freq);
            wxGetApp().getAppFrame()->refreshGainUI();
        }

        Close();
        break;
    case WXK_ESCAPE:
        Close();
        break;
    }

    std::string allowed("0123456789.MKGHZmkghz");

    // Support '-' for range
    if (targetMode == FDIALOG_TARGET_DEFAULT && !activeDemod && strValue.length() > 0) {
        allowed.append("-");
    }

    if (allowed.find_first_of(c) != std::string::npos || c == WXK_DELETE || c == WXK_BACK || c == WXK_NUMPAD_DECIMAL
            || (c >= WXK_NUMPAD0 && c <= WXK_NUMPAD9)) {
#if defined(__linux__) || defined(__FreeBSD__)
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

void FrequencyDialog::OnShow(wxShowEvent &event) {
	if (initialString.length() == 1) {
	    dialogText->SetFocus();
	    dialogText->SetSelection(2, 2);
	}
	event.Skip();
}
