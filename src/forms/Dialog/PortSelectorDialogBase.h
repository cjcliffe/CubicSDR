///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PortSelectorDialogBase
///////////////////////////////////////////////////////////////////////////////
class PortSelectorDialogBase : public wxDialog
{
	private:

	protected:
		wxStaticText* m_staticText1;
		wxListBox* m_portList;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_portSelection;
		wxButton* m_cancelButton;
		wxButton* m_okButton;

		// Virtual event handlers, overide them in your derived class
		virtual void onListSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCancelButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOKButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		PortSelectorDialogBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Select Port"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 320,260 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~PortSelectorDialogBase();

};

