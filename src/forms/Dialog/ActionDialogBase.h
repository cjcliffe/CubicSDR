///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 27 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __ACTIONDIALOGBASE_H__
#define __ACTIONDIALOGBASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class ActionDialogBase
///////////////////////////////////////////////////////////////////////////////
class ActionDialogBase : public wxDialog
{
private:

protected:
	wxStaticText* m_questionText;
	wxButton* m_cancelButton;
	wxButton* m_okButton;

	// Virtual event handlers, overide them in your derived class
	virtual void onClickCancel(wxCommandEvent& event) { event.Skip(); }
	virtual void onClickOK(wxCommandEvent& event) { event.Skip(); }


public:

	ActionDialogBase(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("QuestionTitle"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE);
	~ActionDialogBase();

};

#endif //__ACTIONDIALOGBASE_H__
