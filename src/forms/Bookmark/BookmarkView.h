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

    ~TreeViewItem() override = default;;
    
    TreeViewItemType type;
    
    BookmarkEntryPtr bookmarkEnt; 
    BookmarkRangeEntryPtr rangeEnt;
    
    DemodulatorInstancePtr demod;
    std::string groupName;
};


class BookmarkViewVisualDragItem : public wxDialog {
public:
    explicit BookmarkViewVisualDragItem(const wxString& labelValue = L"Popup");
};



class BookmarkView : public BookmarkPanel {
public:
    explicit BookmarkView( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1, -1 ), long style = wxTAB_TRAVERSAL );

    ~BookmarkView() override;
    
    //order an asynchronous refresh/rebuild of the whole tree,
    //will take effect at the next onUpdateTimer() occurence.
    void updateActiveList();
    
    //order asynchronous updates of the bookmarks,
    //will take effect at the next onUpdateTimer() occurence.
    void updateBookmarks();
    void updateBookmarks(const std::string& group);

    bool isKeywordMatch(std::wstring str, std::vector<std::wstring> &keywords);
   
    wxTreeItemId refreshBookmarks();
    void updateTheme();
    void onMenuItem(wxCommandEvent& event);
    bool isMouseInView();
    
    bool getExpandState(const std::string& branchName);
    void setExpandState(const std::string& branchName, bool state);
    
    static BookmarkRangeEntryPtr makeActiveRangeEntry();

protected:
    void activeSelection(const DemodulatorInstancePtr& dsel);
    void bookmarkSelection(const BookmarkEntryPtr& bmSel);
    void rangeSelection(const BookmarkRangeEntryPtr& re);
    
    void activateBookmark(const BookmarkEntryPtr& bmEnt);

    void activateRange(const BookmarkRangeEntryPtr& rangeEnt);
    void recentSelection(const BookmarkEntryPtr& bmSel);
    void groupSelection(const std::string& groupName);
    void bookmarkBranchSelection();
    void recentBranchSelection();
    void rangeBranchSelection();
    void activeBranchSelection();

    void ensureSelectionInView();
    void hideProps(bool hidePanel = true);
    void showProps();
    
    void onUpdateTimer( wxTimerEvent& event ) override;

    //refresh / rebuild the whole tree item immediatly
    void doUpdateActiveList();

    void onKeyUp( wxKeyEvent& event ) override;
    void onTreeActivate( wxTreeEvent& event ) override;
    void onTreeCollapse( wxTreeEvent& event ) override;
    void onTreeExpanded( wxTreeEvent& event ) override;
    void onTreeItemMenu( wxTreeEvent& event ) override;
    void onTreeSelect( wxTreeEvent& event ) override;
    void onTreeSelectChanging( wxTreeEvent& event ) override;
    void onLabelKillFocus(wxFocusEvent& event ) override;
    void onLabelText( wxCommandEvent& event ) override;
    void onDoubleClickFreq( wxMouseEvent& event ) override;
    void onDoubleClickBandwidth( wxMouseEvent& event ) override;
    void onTreeBeginDrag( wxTreeEvent& event ) override;
    void onTreeEndDrag( wxTreeEvent& event ) override;
    void onTreeItemGetTooltip( wxTreeEvent& event ) override;
    void onEnterWindow( wxMouseEvent& event ) override;
    void onLeaveWindow( wxMouseEvent& event ) override;
    void onMotion( wxMouseEvent& event ) override;

    void onSearchTextFocus( wxMouseEvent& event ) override;
    void onSearchText( wxCommandEvent& event ) override;
    void onClearSearch( wxCommandEvent& event ) override;
    
    void clearButtons();
    void showButtons();
    void refreshLayout();

    wxButton *makeButton(wxWindow *parent, const std::string& labelVal, wxObjectEventFunction handler);
    wxButton *addButton(wxWindow *parent, const std::string& labelVal, wxObjectEventFunction handler);

    void doBookmarkActive(const std::string& group, const DemodulatorInstancePtr& demod);
    void doBookmarkRecent(const std::string& group, const BookmarkEntryPtr& be);
    void doMoveBookmark(const BookmarkEntryPtr& be, const std::string& group);
    void doRemoveActive(const DemodulatorInstancePtr& demod);
    void doRemoveRecent(const BookmarkEntryPtr& be);
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

    bool skipEvents();
     
    TreeViewItem *itemToTVI(wxTreeItemId item);
    
    void SetTreeItemData(const wxTreeItemId& item, wxTreeItemData *data);

    MouseTracker mouseTracker;
    
    wxTreeItemId rootBranch, activeBranch, bookmarkBranch, recentBranch, rangeBranch;
    
    std::map<std::string, bool> expandState;
    
    TreeViewItem *dragItem;
    wxTreeItemId dragItemId;
    BookmarkViewVisualDragItem *visualDragItem;
    
    // Bookmarks
    std::atomic_bool doUpdateBookmarks;
    std::set< std::string > doUpdateBookmarkGroup;
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

    void setStatusText(const std::string& statusText);

};
