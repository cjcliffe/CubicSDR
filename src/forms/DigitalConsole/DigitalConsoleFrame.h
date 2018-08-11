///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  8 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIGITALCONSOLEFRAME_H__
#define __DIGITALCONSOLEFRAME_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/timer.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DigitalConsoleFrame
///////////////////////////////////////////////////////////////////////////////
class DigitalConsoleFrame : public wxFrame 
{
	private:
	
	protected:
		wxTextCtrl* m_dataView;
		wxButton* m_clearButton;
		wxButton* m_copyButton;
		wxButton* m_pauseButton;
		wxTimer m_refreshTimer;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnClear( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCopy( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPause( wxCommandEvent& event ) { event.Skip(); }
		virtual void DoRefresh( wxTimerEvent& event ) { event.Skip(); }
		
	
	public:
		
		DigitalConsoleFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Digital Output"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 441,394 ), long style = wxCAPTION|wxFRAME_FLOAT_ON_PARENT|wxMAXIMIZE|wxMAXIMIZE_BOX|wxMINIMIZE|wxMINIMIZE_BOX|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL );
		
		~DigitalConsoleFrame();
	
};

#endif //__DIGITALCONSOLEFRAME_H__
