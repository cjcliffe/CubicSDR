///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  8 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __SDRDEVICEADDFORM_H__
#define __SDRDEVICEADDFORM_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class SDRDeviceAddForm
///////////////////////////////////////////////////////////////////////////////
class SDRDeviceAddForm : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText4;
		wxChoice* m_soapyModule;
		wxStaticText* m_paramLabel;
		wxTextCtrl* m_paramText;
		wxButton* m_cancelButton;
		wxButton* m_OkButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSoapyModuleChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkButton( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		SDRDeviceAddForm( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Add SoapySDR Device"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 395,293 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~SDRDeviceAddForm();
	
};

#endif //__SDRDEVICEADDFORM_H__
