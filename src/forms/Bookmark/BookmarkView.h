#pragma once

#include "BookmarkPanel.h"
#include "BookmarkMgr.h"
#include "wx/choice.h"
#include "wx/dialog.h"

class TreeViewItem : public wxTreeItemData {
public:
    enum TreeViewItemType {
        TREEVIEW_ITEM_TYPE_GROUP,
        TREEVIEW_ITEM_TYPE_ACTIVE,
        TREEVIEW_ITEM_TYPE_RECENT,
        TREEVIEW_ITEM_TYPE_BOOKMARK
    };
    
    TreeViewItem() {
        bookmarkEnt = nullptr;
        demod = nullptr;
    };
    
    TreeViewItemType type;
    BookmarkEntry *bookmarkEnt;
    DemodulatorInstance *demod;
    std::string groupName;
};


class BookmarkViewVisualDragItem : public wxDialog {
public:
    BookmarkViewVisualDragItem(wxString labelValue = L"Popup");
};



class BookmarkView : public BookmarkPanel {
public:
    BookmarkView( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1, -1 ), long style = wxTAB_TRAVERSAL );
    
    void updateActiveList();
    void updateBookmarks();
    void updateBookmarks(std::string group);
    void activeSelection(DemodulatorInstance *dsel);
    void bookmarkSelection(BookmarkEntry *bmSel);
    void activateBookmark(BookmarkEntry *bmEnt);
    void recentSelection(BookmarkEntry *bmSel);
    void groupSelection(std::string groupName);
    void bookmarkBranchSelection();
    void recentBranchSelection();
    void activeBranchSelection();
    
    wxTreeItemId refreshBookmarks();
    void updateTheme();
    void onMenuItem(wxCommandEvent& event);
    bool isMouseInView();
    
    
protected:
    
    void hideProps();
    void showProps();
    
    void onUpdateTimer( wxTimerEvent& event );

    std::wstring getBookmarkEntryDisplayName(BookmarkEntry *bmEnt);
    std::wstring getActiveDisplayName(DemodulatorInstance *demod);
    void doUpdateActiveList();

    void onTreeBeginLabelEdit( wxTreeEvent& event );
    void onTreeEndLabelEdit( wxTreeEvent& event );
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

    
    void clearButtons();
    void showButtons();
    void refreshLayout();

    wxButton *makeButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler);
    wxButton *addButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler);

    void doBookmarkActive(std::string group, DemodulatorInstance *demod);
    void doBookmarkRecent(std::string group, BookmarkEntry *be);
    void doMoveBookmark(BookmarkEntry *be, std::string group);
    void doRemoveActive(DemodulatorInstance *demod);
        
    void updateBookmarkChoices();
    void addBookmarkChoice(wxWindow *parent);    
    void onBookmarkChoice( wxCommandEvent &event );
    
    void onRemoveActive( wxCommandEvent& event );
    void onRemoveBookmark( wxCommandEvent& event );
    
    void onActivateBookmark( wxCommandEvent& event );
    void onActivateRecent( wxCommandEvent& event );
    
    void onAddGroup( wxCommandEvent& event );
    void onRemoveGroup( wxCommandEvent& event );
    void onRenameGroup( wxCommandEvent& event );
    
    TreeViewItem *itemToTVI(wxTreeItemId item);

    
    std::atomic_bool mouseInView;
    
    wxTreeItemId rootBranch, activeBranch, bookmarkBranch, recentBranch;
    
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
    BookmarkEntry *nextEnt;
    DemodulatorInstance *nextDemod;
};
