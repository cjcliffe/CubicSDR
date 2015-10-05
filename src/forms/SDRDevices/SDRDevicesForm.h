///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 23 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __SDRDEVICESFORM_H__
#define __SDRDEVICESFORM_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/statusbr.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/treectrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/listctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/timer.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class devFrame
///////////////////////////////////////////////////////////////////////////////
class devFrame : public wxFrame 
{
	private:
	
	protected:
		wxStatusBar* devStatusBar;
		wxPanel* m_panel3;
		wxPanel* m_panel6;
		wxTreeCtrl* devTree;
		wxPanel* m_panel4;
		wxButton* m_addRemoteButton;
		wxButton* m_useSelectedButton;
		wxNotebook* devTabs;
		wxPanel* devInfoPanel;
		wxListCtrl* m_DevInfoList;
		wxPanel* devParamsPanel;
		wxListCtrl* m_ParamInfoList;
		wxTimer m_deviceTimer;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnTreeDoubleClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnDeleteItem( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnSelectionChanged( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnAddRemote( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnUseSelected( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnDeviceTimer( wxTimerEvent& event ) { event.Skip(); }
		
	
	public:
		
		devFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("CubicSDR :: SDR Devices"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 392,467 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~devFrame();
	
};

#endif //__SDRDEVICESFORM_H__
