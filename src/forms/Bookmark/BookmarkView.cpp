#include "BookmarkView.h"
#include "CubicSDR.h"

#include <wx/menu.h>
#include <wx/textdlg.h>
#include <algorithm>
#define wxCONTEXT_ADD_GROUP_ID 1000

BookmarkView::BookmarkView( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : BookmarkPanel(parent, id, pos, size, style) {

    rootBranch = m_treeView->AddRoot("Root");
    activeBranch = m_treeView->AppendItem(rootBranch, "Active");
    bookmarkBranch = m_treeView->AppendItem(rootBranch, "Bookmarks");
    recentBranch = m_treeView->AppendItem(rootBranch, "Recents");
    
    doUpdateActive.store(true);
    doUpdateBookmarks.store(true);
    activeSel = nullptr;
    recentSel = nullptr;
    dragItem = nullptr;
    dragItemId = nullptr;
    
    hideProps();
    m_propPanel->Hide();
    
    m_updateTimer.Start(500);
//    m_treeView->SetDropEffectAboveItem();
    mouseInView.store(false);

}


void BookmarkView::onUpdateTimer( wxTimerEvent& event ) {
    if (doUpdateActive.load()) {
        doUpdateActiveList();
        
        doUpdateActive.store(false);
    }
    if (doUpdateBookmarks.load()) {
        wxTreeItemId bmSel = refreshBookmarks();
        if (bmSel) {
            m_treeView->SelectItem(bmSel);
        }
        doUpdateBookmarks.store(false);
    }
}


void BookmarkView::updateTheme() {
    wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
    wxColour textColor(ThemeMgr::mgr.currentTheme->text);
    wxColour btn(ThemeMgr::mgr.currentTheme->button);
    wxColour btnHl(ThemeMgr::mgr.currentTheme->buttonHighlight);
    
    this->SetBackgroundColour(ThemeMgr::mgr.currentTheme->generalBackground * 4.0);

    m_treeView->SetBackgroundColour(bgColor);
    m_treeView->SetForegroundColour(textColor);
    
    m_propPanel->SetBackgroundColour(bgColor);
    m_propPanel->SetForegroundColour(textColor);
    
    m_labelLabel->SetForegroundColour(textColor);
    m_frequencyVal->SetForegroundColour(textColor);
    m_frequencyLabel->SetForegroundColour(textColor);
    m_bandwidthVal->SetForegroundColour(textColor);
    m_bandwidthLabel->SetForegroundColour(textColor);
    m_modulationVal->SetForegroundColour(textColor);
    m_modulationLabel->SetForegroundColour(textColor);

    m_buttonPanel->SetBackgroundColour(bgColor);
    for (auto p : m_buttonPanel->GetChildren()) {
        p->SetBackgroundColour(bgColor);
    }
}


void BookmarkView::updateActiveList() {
    doUpdateActive.store(true);
}


void BookmarkView::updateBookmarks() {
    doUpdateBookmarks.store(true);
}


void BookmarkView::updateBookmarks(std::string group) {
    doUpdateBookmarkGroup.insert(group);
    doUpdateBookmarks.store(true);
}


wxTreeItemId BookmarkView::refreshBookmarks() {
    groupNames = wxGetApp().getBookmarkMgr().getGroups();
    if (!groupNames.size()) {
        wxGetApp().getBookmarkMgr().getGroup("Ungrouped");
        groupNames = wxGetApp().getBookmarkMgr().getGroups();
    }
    if (doUpdateBookmarkGroup.size()) { // Nothing for the moment..
        doUpdateBookmarkGroup.erase(doUpdateBookmarkGroup.begin(), doUpdateBookmarkGroup.end());
    }

    wxTreeItemId bmSelFound = nullptr;
    for (auto gn_i : groupNames) {
        if (groups.find(gn_i) == groups.end()) {
            TreeViewItem* tvi = new TreeViewItem();
            tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP;
            tvi->groupName = gn_i;
            wxTreeItemId itm = m_treeView->AppendItem(bookmarkBranch, gn_i);
            m_treeView->SetItemData(itm, tvi);
            groups[gn_i] = itm;
        }
        
        wxTreeItemId groupItem = groups[gn_i];
        m_treeView->DeleteChildren(groupItem);
        
        BookmarkGroup bmList = wxGetApp().getBookmarkMgr().getGroup(gn_i);
        for (auto bmEnt : bmList) {
            TreeViewItem* tvi = new TreeViewItem();
            tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK;
            tvi->bookmarkEnt = bmEnt;
            tvi->groupName = gn_i;
            wxTreeItemId itm = m_treeView->AppendItem(groupItem, bmEnt->label);
            m_treeView->SetItemData(itm, tvi);
            if (bookmarkSel == bmEnt) {
                bmSelFound = itm;
            }
        }
    }
    
    return bmSelFound;
}


void BookmarkView::doUpdateActiveList() {
    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();
    
    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
//    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    // Actives
    m_treeView->DeleteChildren(activeBranch);
    
    wxTreeItemId selItem = nullptr;
    for (auto demod_i : demods) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE;
        tvi->demod = demod_i;
        
        wxTreeItemId itm = m_treeView->AppendItem(activeBranch,demod_i->getLabel());
        m_treeView->SetItemData(itm, tvi);
        
        if (activeDemodulator) {
            if (activeDemodulator == demod_i) {
                selItem = itm;
                activeSel = demod_i;
            }
        }
        else if (activeSel == demod_i) {
            selItem = itm;
        }
    }
    
    // Recents
    BookmarkList bmRecents = wxGetApp().getBookmarkMgr().getRecents();
    m_treeView->DeleteChildren(recentBranch);
    
    for (auto bmr_i: bmRecents) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT;
        tvi->bookmarkEnt = bmr_i;

        wxTreeItemId itm = m_treeView->AppendItem(recentBranch, bmr_i->label);
        m_treeView->SetItemData(itm, tvi);
        if (recentSel == bmr_i) {
            selItem = itm;
        }
    }

    if (selItem != nullptr) {
        m_treeView->SelectItem(selItem);
    }
}


void BookmarkView::onTreeBeginLabelEdit( wxTreeEvent& event ) {
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));

    if (!tvi) {
        event.Veto();
        return;
    }
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        event.Allow();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        event.Veto();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        event.Allow();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        event.Allow();
    } else {
        event.Veto();
    }
}


void BookmarkView::onTreeEndLabelEdit( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onTreeActivate( wxTreeEvent& event ) {
    if (recentSel) {
        activateBookmark(recentSel);
    }
    if (bookmarkSel) {
        activateBookmark(bookmarkSel);
    }
}


void BookmarkView::onTreeCollapse( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onTreeExpanded( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onTreeItemMenu( wxTreeEvent& event ) {
    if (m_treeView->GetSelection() == bookmarkBranch) {
        wxMenu menu;
        menu.Append(wxCONTEXT_ADD_GROUP_ID, "Add Group");
        menu.Connect(wxCONTEXT_ADD_GROUP_ID, wxEVT_MENU, (wxObjectEventFunction)&BookmarkView::onMenuItem);
        PopupMenu(&menu);
    }
}


void BookmarkView::onMenuItem(wxCommandEvent& event) {
    if (event.GetId() == wxCONTEXT_ADD_GROUP_ID) {
        wxString stringVal = wxGetTextFromUser("Enter Group Name", "Add Group", "");
        if (stringVal.ToStdString() != "") {
            wxGetApp().getBookmarkMgr().getGroup(stringVal.ToStdString());
            wxGetApp().getBookmarkMgr().updateBookmarks();
        }
    }
}


bool BookmarkView::isMouseInView() {
    return mouseInView.load();
}


void BookmarkView::hideProps() {
    m_frequencyLabel->Hide();
    m_frequencyVal->Hide();
    
    m_bandwidthLabel->Hide();
    m_bandwidthVal->Hide();
    
    m_modulationVal->Hide();
    m_modulationLabel->Hide();
    
    m_labelText->Hide();
    m_labelLabel->Hide();
    
    m_buttonPanel->Hide();
}


void BookmarkView::clearButtons() {
    m_buttonPanel->DestroyChildren();
    m_buttonPanel->Hide();
}

void BookmarkView::showButtons() {
    m_buttonPanel->Show();
//    m_buttonPanel->Layout();
    m_buttonPanel->GetSizer()->Layout();
}


wxButton *BookmarkView::makeButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler) {
    wxButton *nButton = new wxButton( m_buttonPanel, wxID_ANY, labelVal);
    nButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, handler, NULL, this);
    nButton->SetBackgroundColour(ThemeMgr::mgr.currentTheme->generalBackground);
    return nButton;
}


wxButton *BookmarkView::addButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler) {
    wxButton *nButton = makeButton(parent, labelVal, handler);
    parent->GetSizer()->Add( nButton, 0, wxEXPAND);
    return nButton;
}


void BookmarkView::activeSelection(DemodulatorInstance *dsel) {
    activeSel = dsel;
    recentSel = nullptr;
    
    m_frequencyVal->SetLabelText(frequencyToStr(dsel->getFrequency()));
    m_bandwidthVal->SetLabelText(frequencyToStr(dsel->getBandwidth()));
    m_modulationVal->SetLabelText(dsel->getDemodulatorType());
    m_labelText->SetValue(dsel->getLabel());
    
    hideProps();
    
    m_frequencyVal->Show();
    m_frequencyLabel->Show();
    
    m_bandwidthVal->Show();
    m_bandwidthLabel->Show();
    
    m_modulationVal->Show();
    m_modulationLabel->Show();
    
    m_labelText->Show();
    m_labelLabel->Show();
    
    clearButtons();
    
    addButton(m_buttonPanel, "Bookmark Active", wxCommandEventHandler( BookmarkView::onBookmarkActive ));
    addButton(m_buttonPanel, "Remove Active", wxCommandEventHandler( BookmarkView::onRemoveActive ));

    showButtons();

    this->Layout();
}


void BookmarkView::activateBookmark(BookmarkEntry *bmEnt) {
    DemodulatorInstance *newDemod = wxGetApp().getDemodMgr().loadInstance(bmEnt->node);
    newDemod->run();
    newDemod->setActive(true);
    wxGetApp().bindDemodulator(newDemod);
    if (bmEnt == recentSel) {
        activeSel = newDemod;
        recentSel = nullptr;
    }
    doUpdateActiveList();
}


void BookmarkView::bookmarkSelection(BookmarkEntry *bmSel) {
    bookmarkSel = bmSel;
    recentSel = nullptr;
    activeSel = nullptr;
    
    m_frequencyVal->SetLabelText(frequencyToStr(bmSel->frequency));
    m_bandwidthVal->SetLabelText(frequencyToStr(bmSel->bandwidth));
    m_modulationVal->SetLabelText(bmSel->type);
    m_labelText->SetValue(bmSel->label);
    
    hideProps();
    
    m_frequencyVal->Show();
    m_frequencyLabel->Show();
    
    m_bandwidthVal->Show();
    m_bandwidthLabel->Show();
    
    m_modulationVal->Show();
    m_modulationLabel->Show();
    
    m_labelText->Show();
    m_labelLabel->Show();
    
    clearButtons();
    
    addButton(m_buttonPanel, "Activate Bookmark", wxCommandEventHandler( BookmarkView::onActivateBookmark ));
    addButton(m_buttonPanel, "Remove Bookmark", wxCommandEventHandler( BookmarkView::onRemoveBookmark ));
    
    showButtons();

    this->Layout();
}


void BookmarkView::recentSelection(BookmarkEntry *bmSel) {
    recentSel = bmSel;
    activeSel = nullptr;
    bookmarkSel = nullptr;
    
    m_frequencyVal->SetLabelText(frequencyToStr(bmSel->frequency));
    m_bandwidthVal->SetLabelText(frequencyToStr(bmSel->bandwidth));
    m_modulationVal->SetLabelText(bmSel->type);
    m_labelText->SetValue(bmSel->label);
    
    hideProps();
    
    m_frequencyVal->Show();
    m_frequencyLabel->Show();
    
    m_bandwidthVal->Show();
    m_bandwidthLabel->Show();
    
    m_modulationVal->Show();
    m_modulationLabel->Show();
    
    m_labelText->Show();
    m_labelLabel->Show();
    
    clearButtons();
    
    addButton(m_buttonPanel, "Activate Recent", wxCommandEventHandler( BookmarkView::onActivateRecent ));
    addButton(m_buttonPanel, "Bookmark Recent", wxCommandEventHandler( BookmarkView::onBookmarkRecent ));
    
    showButtons();

    this->Layout();
}


void BookmarkView::onTreeSelect( wxTreeEvent& event ) {
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));

    if (!tvi) {
        m_propPanel->Hide();
        hideProps();
        this->Layout();
        return;
    }
                                                    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        m_propPanel->Show();
        activeSelection(tvi->demod);
        wxGetApp().getDemodMgr().setActiveDemodulator(tvi->demod, false);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        m_propPanel->Show();
        recentSelection(tvi->bookmarkEnt);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        m_propPanel->Show();
        bookmarkSelection(tvi->bookmarkEnt);
    } else {
        m_propPanel->Hide();
        hideProps();
        this->Layout();
    }
}


void BookmarkView::onTreeSelectChanging( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onLabelText( wxCommandEvent& event ) {
    event.Skip();
}


void BookmarkView::onDoubleClickFreq( wxMouseEvent& event ) {
    if (activeSel) {
        wxGetApp().getDemodMgr().setActiveDemodulator(activeSel, false);
        wxGetApp().showFrequencyInput(FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_DEFAULT);
    }
}


void BookmarkView::onDoubleClickBandwidth( wxMouseEvent& event ) {
    if (activeSel) {
        wxGetApp().getDemodMgr().setActiveDemodulator(activeSel, false);
        wxGetApp().showFrequencyInput(FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_BANDWIDTH);
    }
}


void BookmarkView::onBookmarkActive( wxCommandEvent& event ) {
    if (activeSel) {
        wxGetApp().getBookmarkMgr().addBookmark("Ungrouped", activeSel);
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
}

void BookmarkView::onBookmarkRecent( wxCommandEvent& event ) {
    if (bookmarkSel) {
        wxGetApp().getBookmarkMgr().removeRecent(bookmarkSel);
        wxGetApp().getBookmarkMgr().addBookmark("Ungrouped", bookmarkSel);
    }
}


void BookmarkView::onRemoveActive( wxCommandEvent& event ) {
    if (activeSel != nullptr) {
        wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, false);
        wxGetApp().removeDemodulator(activeSel);
        wxGetApp().getDemodMgr().deleteThread(activeSel);
        activeSel = nullptr;
    }
}


void BookmarkView::onRemoveBookmark( wxCommandEvent& event ) {
    // todo
}


void BookmarkView::onActivateBookmark( wxCommandEvent& event ) {
    if (bookmarkSel) {
        activateBookmark(bookmarkSel);
    }
}


void BookmarkView::onActivateRecent( wxCommandEvent& event ) {
    if (recentSel) {
        activateBookmark(recentSel);
    }
}


void BookmarkView::onTreeBeginDrag( wxTreeEvent& event ) {
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));
    
    dragItem = nullptr;
    dragItemId = nullptr;
    
    if (!tvi) {
        event.Veto();
        return;
    }
    
    bool bAllow = false;
    std::string dragItemName;
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        bAllow = true;
        dragItemName = tvi->demod->getLabel();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        bAllow = true;
        dragItemName = tvi->bookmarkEnt->label;
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        bAllow = true;
        dragItemName = tvi->bookmarkEnt->label;
    }
    
    if (bAllow) {
        wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
        wxColour textColor(ThemeMgr::mgr.currentTheme->text);
        
        m_treeView->SetBackgroundColour(textColor);
        m_treeView->SetForegroundColour(bgColor);
        m_treeView->SetToolTip("Dragging " + dragItemName);
        
        dragItem = tvi;
        dragItemId = event.GetItem();

        event.Allow();
    } else {
        event.Veto();
    }
}


void BookmarkView::onTreeEndDrag( wxTreeEvent& event ) {

    wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
    wxColour textColor(ThemeMgr::mgr.currentTheme->text);
    
    m_treeView->SetBackgroundColour(bgColor);
    m_treeView->SetForegroundColour(textColor);
    m_treeView->UnsetToolTip();

    if (!event.GetItem()) {
        event.Veto();
        return;
    }

    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));

    if (!tvi) {
        if (event.GetItem() == bookmarkBranch) {
            if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
                wxGetApp().getBookmarkMgr().addBookmark("Ungrouped", dragItem->demod);
                wxGetApp().getBookmarkMgr().updateBookmarks();
            }else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
                wxGetApp().getBookmarkMgr().removeRecent(dragItem->bookmarkEnt);
                wxGetApp().getBookmarkMgr().addBookmark("Ungrouped", dragItem->bookmarkEnt);
                m_treeView->Delete(dragItemId);
                wxGetApp().getBookmarkMgr().updateBookmarks();
                wxGetApp().getBookmarkMgr().updateActiveList();
            }
        }
        
        return;
    }
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) { // Active -> Group Item
            wxGetApp().getBookmarkMgr().addBookmark(tvi->groupName, dragItem->demod);
            wxGetApp().getBookmarkMgr().updateBookmarks();
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) { // Recent -> Group Item
            wxGetApp().getBookmarkMgr().removeRecent(dragItem->bookmarkEnt);
            wxGetApp().getBookmarkMgr().addBookmark(tvi->groupName, dragItem->bookmarkEnt);
            m_treeView->Delete(dragItemId);
            wxGetApp().getBookmarkMgr().updateBookmarks();
            wxGetApp().getBookmarkMgr().updateActiveList();
        }
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) { // Active -> Same Group
            wxGetApp().getBookmarkMgr().addBookmark(tvi->groupName, dragItem->demod);
            wxGetApp().getBookmarkMgr().updateBookmarks();
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) { // Recent -> Same Group
            wxGetApp().getBookmarkMgr().removeRecent(dragItem->bookmarkEnt);
            wxGetApp().getBookmarkMgr().addBookmark(tvi->groupName, dragItem->bookmarkEnt);
            m_treeView->Delete(dragItemId);
            wxGetApp().getBookmarkMgr().updateBookmarks();
            wxGetApp().getBookmarkMgr().updateActiveList();
        }
    }
}


void BookmarkView::onTreeDeleteItem( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onTreeItemGetTooltip( wxTreeEvent& event ) {
    
    event.Skip();
}


void BookmarkView::onEnterWindow( wxMouseEvent& event ) {
    mouseInView.store(true);
}


void BookmarkView::onLeaveWindow( wxMouseEvent& event ) {
    mouseInView.store(false);
}
