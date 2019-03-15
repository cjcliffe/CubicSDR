// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "DemodLabelDialog.h"

#include "wx/clipbrd.h"
#include <sstream>
#include "CubicSDR.h"

wxBEGIN_EVENT_TABLE(DemodLabelDialog, wxDialog)
EVT_CHAR_HOOK(DemodLabelDialog::OnChar)
EVT_SHOW(DemodLabelDialog::OnShow)
wxEND_EVENT_TABLE()

DemodLabelDialog::DemodLabelDialog(wxWindow * parent, wxWindowID id, const wxString & title, 
        DemodulatorInstancePtr demod, const wxPoint & position,
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

    //we support 16 bit strings for user labels internally.
    wxString strValue = dialogText->GetValue();

    switch (c) {
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:

        //No need to display the demodulator type twice if the user do not change the default value...
        //when comparing getDemodulatorType() std::string, take care of "upgrading" it to wxString which will 
        //try to its best... 
        if (strValue != wxString(activeDemod->getDemodulatorType())) {
            activeDemod->setDemodulatorUserLabel(strValue.ToStdWstring());
        }
        else {
            activeDemod->setDemodulatorUserLabel(L"");
        }
        wxGetApp().getBookmarkMgr().updateActiveList();
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
        std::wstring clipText = data.GetText().ToStdWstring();
        wxTheClipboard->SetData(new wxTextDataObject(clipText));
        wxTheClipboard->Close();
        event.Skip();
    }
    else if (c == WXK_RIGHT || c == WXK_LEFT || event.ControlDown()) {
        event.Skip();

    }
    else {
#if defined(__linux__) || defined(__FreeBSD__)
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
