///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 23 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __BOOKMARKPANEL_H__
#define __BOOKMARKPANEL_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/treectrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/timer.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class BookmarkPanel
///////////////////////////////////////////////////////////////////////////////
class BookmarkPanel : public wxPanel 
{
	private:
	
	protected:
		wxTreeCtrl* m_treeView;
		wxPanel* m_propPanel;
		wxStaticText* m_labelLabel;
		wxTextCtrl* m_labelText;
		wxStaticText* m_frequencyLabel;
		wxStaticText* m_frequencyVal;
		wxStaticText* m_bandwidthLabel;
		wxStaticText* m_bandwidthVal;
		wxStaticText* m_modulationLabel;
		wxStaticText* m_modulationVal;
		wxButton* m_bookmarkButton;
		wxButton* m_activateButton;
		wxButton* m_removeButton;
		wxTimer m_updateTimer;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onTreeBeginLabelEdit( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeEndLabelEdit( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeActivate( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeCollapse( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeExpanded( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeItemMenu( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeSelect( wxTreeEvent& event ) { event.Skip(); }
		virtual void onTreeSelectChanging( wxTreeEvent& event ) { event.Skip(); }
		virtual void onLabelText( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDoubleClickFreq( wxMouseEvent& event ) { event.Skip(); }
		virtual void onDoubleClickBandwidth( wxMouseEvent& event ) { event.Skip(); }
		virtual void onBookmark( wxCommandEvent& event ) { event.Skip(); }
		virtual void onActivate( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRemove( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateTimer( wxTimerEvent& event ) { event.Skip(); }
		
	
	public:
		
		BookmarkPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 169,471 ), long style = wxTAB_TRAVERSAL ); 
		~BookmarkPanel();
	
};

#endif //__BOOKMARKPANEL_H__
