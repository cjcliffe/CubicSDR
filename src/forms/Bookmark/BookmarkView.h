// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "wx/choice.h"
#include "wx/dialog.h"

#include "BookmarkPanel.h"
#include "BookmarkMgr.h"
#include "MouseTracker.h"

class TreeViewItem : public wxTreeItemData {
public:
    enum TreeViewItemType {
        TREEVIEW_ITEM_TYPE_GROUP,
        TREEVIEW_ITEM_TYPE_ACTIVE,
        TREEVIEW_ITEM_TYPE_RECENT,
        TREEVIEW_ITEM_TYPE_BOOKMARK,
        TREEVIEW_ITEM_TYPE_RANGE
    };
    
    TreeViewItem() {
        demod = nullptr;
        bookmarkEnt = nullptr;
        rangeEnt = nullptr;
    };
    // copy constructor
    TreeViewItem(const TreeViewItem& src) {
        demod = src.demod;
        bookmarkEnt = src.bookmarkEnt;
        rangeEnt = src.rangeEnt;
        type = src.type;
        groupName = src.groupName;
    };

    virtual ~TreeViewItem() {
      //
    };
    
    TreeViewItemType type;
    
    BookmarkEntryPtr bookmarkEnt; 
    BookmarkRangeEntryPtr rangeEnt;
    
    DemodulatorInstancePtr demod;
    std::string groupName;
};


class BookmarkViewVisualDragItem : public wxDialog {
public:
    BookmarkViewVisualDragItem(wxString labelValue = L"Popup");
};



class BookmarkView : public BookmarkPanel {
public:
    BookmarkView( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1, -1 ), long style = wxTAB_TRAVERSAL );

    virtual ~BookmarkView();
    
    //order an asynchronous refresh/rebuild of the whole tree,
    //will take effect at the next onUpdateTimer() occurence.
    void updateActiveList();
    
    //order asynchronous updates of the bookmarks,
    //will take effect at the next onUpdateTimer() occurence.
    void updateBookmarks();
    void updateBookmarks(std::string group);

    bool isKeywordMatch(std::wstring str, std::vector<std::wstring> &keywords);
   
    wxTreeItemId refreshBookmarks();
    void updateTheme();
    void onMenuItem(wxCommandEvent& event);
    bool isMouseInView();
    
    bool getExpandState(std::string branchName);
    void setExpandState(std::string branchName, bool state);
    
    void loadDefaultRanges();
    static BookmarkRangeEntryPtr makeActiveRangeEntry();

protected:
    void activeSelection(DemodulatorInstancePtr dsel);
    void bookmarkSelection(BookmarkEntryPtr bmSel);
    void rangeSelection(BookmarkRangeEntryPtr re);
    
    void activateBookmark(BookmarkEntryPtr bmEnt);

    void activateRange(BookmarkRangeEntryPtr rangeEnt);
    void recentSelection(BookmarkEntryPtr bmSel);
    void groupSelection(std::string groupName);
    void bookmarkBranchSelection();
    void recentBranchSelection();
    void rangeBranchSelection();
    void activeBranchSelection();
    
    void hideProps();
    void showProps();
    
    void onUpdateTimer( wxTimerEvent& event );

    //refresh / rebuild the whole tree item immediatly
    void doUpdateActiveList();

    void onTreeActivate( wxTreeEvent& event );
    void onTreeCollapse( wxTreeEvent& event );
    void onTreeExpanded( wxTreeEvent& event );
    void onTreeItemMenu( wxTreeEvent& event );
    void onTreeSelect( wxTreeEvent& event );
    void onTreeSelectChanging( wxTreeEvent& event );
    void onLabelText( wxCommandEvent& event );
    void onDoubleClickFreq( wxMouseEvent& event );
    void onDoubleClickBandwidth( wxMouseEvent& event );
    void onTreeBeginDrag( wxTreeEvent& event );
    void onTreeEndDrag( wxTreeEvent& event );
    void onTreeItemGetTooltip( wxTreeEvent& event );
    void onEnterWindow( wxMouseEvent& event );
    void onLeaveWindow( wxMouseEvent& event );
    void onMotion( wxMouseEvent& event );

    void onSearchTextFocus( wxMouseEvent& event );
    void onSearchText( wxCommandEvent& event );
    void onClearSearch( wxCommandEvent& event );
    
    void clearButtons();
    void showButtons();
    void refreshLayout();

    wxButton *makeButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler);
    wxButton *addButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler);

    void doBookmarkActive(std::string group, DemodulatorInstancePtr demod);
    void doBookmarkRecent(std::string group, BookmarkEntryPtr be);
    void doMoveBookmark(BookmarkEntryPtr be, std::string group);
    void doRemoveActive(DemodulatorInstancePtr demod);
    void doRemoveRecent(BookmarkEntryPtr be);
    void doClearRecents();
    
    void updateBookmarkChoices();
    void addBookmarkChoice(wxWindow *parent);    
    void onBookmarkChoice( wxCommandEvent &event );
    
    void onRemoveActive( wxCommandEvent& event );
    void onStartRecording( wxCommandEvent& event );
    void onStopRecording( wxCommandEvent& event );
    void onRemoveBookmark( wxCommandEvent& event );
    
    void onActivateBookmark( wxCommandEvent& event );
    void onActivateRecent( wxCommandEvent& event );
    void onRemoveRecent ( wxCommandEvent& event );
    void onClearRecents ( wxCommandEvent& event );
    
    void onAddGroup( wxCommandEvent& event );
    void onRemoveGroup( wxCommandEvent& event );

    void onAddRange( wxCommandEvent& event );
    void onRemoveRange( wxCommandEvent& event );
    void onRenameRange( wxCommandEvent& event );
    void onActivateRange( wxCommandEvent& event );
    void onUpdateRange( wxCommandEvent& event );
    
    TreeViewItem *itemToTVI(wxTreeItemId item);
    
    void SetTreeItemData(const wxTreeItemId& item, wxTreeItemData *data);

    MouseTracker mouseTracker;
    
    wxTreeItemId rootBranch, activeBranch, bookmarkBranch, recentBranch, rangeBranch;
    
    std::map<std::string, bool> expandState;
    
    TreeViewItem *dragItem;
    wxTreeItemId dragItemId;
    BookmarkViewVisualDragItem *visualDragItem;
    
    bool editingLabel;
    
    // Bookmarks
    std::atomic_bool doUpdateBookmarks;
    std::set< std::string > doUpdateBookmarkGroup;
    BookmarkNames groupNames;
    std::map<std::string, wxTreeItemId> groups;
    wxArrayString bookmarkChoices;
    wxChoice *bookmarkChoice;
    
    // Active
    std::atomic_bool doUpdateActive;
    
    // Focus
    BookmarkEntryPtr nextEnt;
    BookmarkRangeEntryPtr nextRange;
    DemodulatorInstancePtr nextDemod;
    std::string nextGroup;
    
    // Search
    std::vector<std::wstring> searchKeywords;

    void setStatusText(std::string statusText);
};
