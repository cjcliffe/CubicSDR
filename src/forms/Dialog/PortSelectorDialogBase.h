///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 27 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PORTSELECTORDIALOGBASE_H__
#define __PORTSELECTORDIALOGBASE_H__

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
#include <wx/button.h>
#include <wx/panel.h>
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
	wxPanel* m_buttonPanel;
	wxButton* m_cancelButton;
	wxButton* m_okButton;

	// Virtual event handlers, overide them in your derived class
	virtual void onListSelect(wxCommandEvent& event) { event.Skip(); }
	virtual void onCancelButton(wxCommandEvent& event) { event.Skip(); }
	virtual void onOKButton(wxCommandEvent& event) { event.Skip(); }


public:

	PortSelectorDialogBase(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Select Port"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(304, 197), long style = wxDEFAULT_DIALOG_STYLE);
	~PortSelectorDialogBase();

};

#endif //__PORTSELECTORDIALOGBASE_H__
