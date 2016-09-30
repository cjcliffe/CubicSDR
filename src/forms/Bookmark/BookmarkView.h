#pragma once

#include "BookmarkPanel.h"

#include "BookmarkMgr.h"

class BookmarkView : public BookmarkPanel {
public:
    BookmarkView( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1, -1 ), long style = wxTAB_TRAVERSAL );
    
    void updateActiveList();
    void activeSelection(DemodulatorInstance *dsel);
    void updateTheme();
    
protected:
    
    void hideProps();
    
    void onUpdateTimer( wxTimerEvent& event );
    void doUpdateActiveList();

    void onTreeBeginLabelEdit( wxTreeEvent& event );
    void onTreeEndLabelEdit( wxTreeEvent& event );
    void onTreeActivate( wxTreeEvent& event );
    void onTreeCollapse( wxTreeEvent& event );
    void onTreeExpanded( wxTreeEvent& event );
    void onTreeSelect( wxTreeEvent& event );
    void onTreeSelectChanging( wxTreeEvent& event );
    void onLabelText( wxCommandEvent& event );
    void onDoubleClickFreq( wxMouseEvent& event );
    void onDoubleClickBandwidth( wxMouseEvent& event );
    void onBookmark( wxCommandEvent& event );
    void onActivate( wxCommandEvent& event );
    void onRemove( wxCommandEvent& event );
    
    bool doUpdateActive;
    wxTreeItemId rootBranch, activeBranch, bookmarkBranch;
    std::map<std::string, wxTreeItemId> groups;
    
    std::map<wxTreeItemId, DemodulatorInstance *> activeItems;
    DemodulatorInstance *activeSel;
};