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
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/menu.h>
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
		wxTreeCtrl* devTree;
		wxNotebook* devTabs;
		wxPanel* devInfoPanel;
		wxListCtrl* m_DevInfoList;
		wxPanel* devParamsPanel;
		wxListCtrl* m_ParamInfoList;
		wxMenuBar* devMenuBar;
		wxMenu* devFileMenu;
		wxMenu* devEditMenu;
	
	public:
		
		devFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("CubicSDR :: SDR Devices"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 692,467 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~devFrame();
	
};

#endif //__SDRDEVICESFORM_H__
