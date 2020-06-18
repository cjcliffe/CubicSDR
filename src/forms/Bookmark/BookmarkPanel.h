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
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/treectrl.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/timer.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class BookmarkPanel
///////////////////////////////////////////////////////////////////////////////
class BookmarkPanel : public wxPanel
{
	private:

	protected:
		wxTextCtrl* m_searchText;
		wxButton* m_clearSearchButton;
		wxTreeCtrl* m_treeView;
		wxStaticLine* m_propPanelDivider;
		wxPanel* m_propPanel;
		wxStaticText* m_labelLabel;
		wxTextCtrl* m_labelText;
		wxStaticText* m_frequencyLabel;
		wxStaticText* m_frequencyVal;
		wxStaticText* m_bandwidthLabel;
		wxStaticText* m_bandwidthVal;
		wxStaticText* m_modulationLabel;
		wxStaticText* m_modulationVal;
		wxPanel* m_buttonPanel;
		wxTimer m_updateTimer;

		// Virtual event handlers, overide them in your derived class
		virtual void onEnterWindow( wxMouseEvent& event ) { event.Skip(); }
		virtual void onLeaveWindow( wxMouseEvent& event ) { event.Skip(); }
		virtual void onMotion( wxMouseEvent& event ) { event.Skip(); }
		virtual void onSearchTextFocus( wxMouseEvent& event ) { event.Skip(); }
		virtual void onSearchText( wxCommandEvent& event ) { event.Skip(); }
		virtual void onClearSearch( wxCommandEvent& event ) { event.Skip(); }
		virtual void onKeyUp( wxKeyEvent& event ) { event.Skip(); }
		virtual void onTreeBeginDrag( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeEndDrag( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeActivate( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeCollapse( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeExpanded( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeItemGetTooltip( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeItemMenu( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeSelect( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeSelectChanging( wxTreeEvent& event ) { event.Skip(); }
		virtual void onLabelKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onLabelText( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDoubleClickFreq( wxMouseEvent& event ) { event.Skip(); }
		virtual void onDoubleClickBandwidth( wxMouseEvent& event ) { event.Skip(); }
		virtual void onUpdateTimer( wxTimerEvent& event ) { event.Skip(); }


	public:

		BookmarkPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 169,471 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~BookmarkPanel();

};

