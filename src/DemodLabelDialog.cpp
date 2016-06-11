#include "DemodLabelDialog.h"

#include "wx/clipbrd.h"
#include <sstream>
#include "CubicSDR.h"

wxBEGIN_EVENT_TABLE(DemodLabelDialog, wxDialog)
EVT_CHAR_HOOK(DemodLabelDialog::OnChar)
EVT_SHOW(DemodLabelDialog::OnShow)
wxEND_EVENT_TABLE()

DemodLabelDialog::DemodLabelDialog(wxWindow * parent, wxWindowID id, const wxString & title, 
        DemodulatorInstance *demod, const wxPoint & position,
        const wxSize & size, long style) :
        wxDialog(parent, id, title, position, size, style) {

    wxString labelStr;

    //by construction, is allways != nullptr
    activeDemod = demod;
	
    labelStr = activeDemod->getDemodulatorUserLabel();

    if (labelStr.empty()) {
        //propose a default value...
        labelStr = activeDemod->getDemodulatorType();
    }
             
                           
    dialogText = new wxTextCtrl(this, wxID_LABEL_INPUT, labelStr, wxPoint(6, 1), wxSize(size.GetWidth() - 20, size.GetHeight() - 70),
    wxTE_PROCESS_ENTER);
    dialogText->SetFont(wxFont(15, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    Centre();

    dialogText->SetValue(labelStr);     
    dialogText->SetSelection(-1, -1);   
}


void DemodLabelDialog::OnChar(wxKeyEvent& event) {
    int c = event.GetKeyCode();

    std::string strValue = dialogText->GetValue().ToStdString();

    switch (c) {
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:

        //No need to display the demodulator type twice if the user do not change the default value... 
        if (strValue != activeDemod->getDemodulatorType()) {
            activeDemod->setDemodulatorUserLabel(strValue);
        }
        else {
            activeDemod->setDemodulatorUserLabel("");
        }

        Close();
        break;
    case WXK_ESCAPE:
        Close();
        break;
    }

    if (event.ControlDown() && c == 'V') {
        // Alter clipboard contents to remove unwanted chars
        wxTheClipboard->Open();
        wxTextDataObject data;
        wxTheClipboard->GetData(data);
        std::string clipText = data.GetText().ToStdString();
        wxTheClipboard->SetData(new wxTextDataObject(clipText));
        wxTheClipboard->Close();
        event.Skip();
    }
    else if (c == WXK_RIGHT || c == WXK_LEFT || event.ControlDown()) {
        event.Skip();

    }
    else {
#ifdef __linux__
        dialogText->OnChar(event);
        event.Skip();
#else
        event.DoAllowNextEvent();
#endif
    }
}

void DemodLabelDialog::OnShow(wxShowEvent &event) {
		
    dialogText->SetFocus();
    dialogText->SetSelection(-1, -1);
	event.Skip();
}
