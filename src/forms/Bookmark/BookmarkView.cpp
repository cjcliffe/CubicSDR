// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <wx/menu.h>
#include <wx/textdlg.h>

#include <algorithm>
#include <wchar.h>

#include "BookmarkView.h"
#include "CubicSDR.h"
#include "ActionDialog.h"


#define wxCONTEXT_ADD_GROUP_ID 1000

#define BOOKMARK_VIEW_CHOICE_DEFAULT "Bookmark.."
#define BOOKMARK_VIEW_CHOICE_MOVE "Move to.."
#define BOOKMARK_VIEW_CHOICE_NEW "(New Group..)"

#define BOOKMARK_VIEW_STR_ADD_GROUP "Add Group"
#define BOOKMARK_VIEW_STR_ADD_GROUP_DESC "Enter Group Name"
#define BOOKMARK_VIEW_STR_UNNAMED "Unnamed"
#define BOOKMARK_VIEW_STR_CLEAR_RECENT "Clear Recents"
#define BOOKMARK_VIEW_STR_RENAME_GROUP "Rename Group"


BookmarkViewVisualDragItem::BookmarkViewVisualDragItem(wxString labelValue) : wxDialog(NULL, wxID_ANY, L"", wxPoint(20,20), wxSize(-1,-1), wxSTAY_ON_TOP | wxALL ) {
    
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText *label = new wxStaticText( this, wxID_ANY, labelValue, wxDefaultPosition, wxDefaultSize, wxEXPAND );
    
    sizer->Add(label, 1, wxALL | wxEXPAND, 5);
    
    SetSizerAndFit(sizer);
    Show();
}

class ActionDialogRemoveBookmark : public ActionDialog {
public:
    ActionDialogRemoveBookmark( BookmarkEntryPtr be ) : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Remove Bookmark?")) {
        subject = be;
        m_questionText->SetLabelText(wxT("Are you sure you want to remove the bookmark\n '" + BookmarkMgr::getBookmarkEntryDisplayName(subject) + "'?"));
    }
    
    void doClickOK() {
        wxGetApp().getBookmarkMgr().removeBookmark(subject);
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }

private:
    BookmarkEntryPtr subject;
};

class ActionDialogRemoveGroup : public ActionDialog {
public:
    ActionDialogRemoveGroup( std::string groupName ) : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Remove Group?")) {
        subject = groupName;
        m_questionText->SetLabelText(wxT("Warning: Are you sure you want to remove the group\n '" + subject + "' AND ALL BOOKMARKS WITHIN IT?"));
    }
    
    void doClickOK() {
        wxGetApp().getBookmarkMgr().removeGroup(subject);
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
    
private:
    std::string subject;
};


class ActionDialogRemoveRange : public ActionDialog {
public:
    ActionDialogRemoveRange( BookmarkRangeEntryPtr rangeEnt ) : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Remove Range?")) {
        subject = rangeEnt;
        
        std::wstring name = rangeEnt->label;
        
        if (name.length() == 0) {
            std::string wstr = frequencyToStr(rangeEnt->startFreq) + " - " + frequencyToStr(rangeEnt->endFreq);
            name = std::wstring(wstr.begin(),wstr.end());
        }
        
        m_questionText->SetLabelText(L"Are you sure you want to remove the range\n '" + name + L"'?");
    }
    
    void doClickOK() {
        wxGetApp().getBookmarkMgr().removeRange(subject);
        wxGetApp().getBookmarkMgr().updateActiveList();
    }
    
private:
    BookmarkRangeEntryPtr subject;
};


class ActionDialogUpdateRange : public ActionDialog {
public:
    ActionDialogUpdateRange( BookmarkRangeEntryPtr rangeEnt ) : ActionDialog(wxGetApp().getAppFrame(), wxID_ANY, wxT("Update Range?")) {
        subject = rangeEnt;
        
        std::wstring name = rangeEnt->label;
        
        if (name.length() == 0) {
            std::string wstr = frequencyToStr(rangeEnt->startFreq) + " - " + frequencyToStr(rangeEnt->endFreq);
            name = std::wstring(wstr.begin(),wstr.end());
        }
        
        m_questionText->SetLabelText(L"Are you sure you want to update the range\n '" + name + L"' to the active range?");
    }
    
    void doClickOK() {
        BookmarkRangeEntryPtr ue = BookmarkView::makeActiveRangeEntry();

        subject->freq = ue->freq;
        subject->startFreq = ue->startFreq;
        subject->endFreq = ue->endFreq;
          
        wxGetApp().getBookmarkMgr().updateActiveList();
    }
    
private:
    BookmarkRangeEntryPtr subject;
};




BookmarkView::BookmarkView( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : BookmarkPanel(parent, id, pos, size, style) {

    rootBranch = m_treeView->AddRoot("Root");
    activeBranch = m_treeView->AppendItem(rootBranch, "Active");
    rangeBranch = m_treeView->AppendItem(rootBranch, "View Ranges");
    bookmarkBranch = m_treeView->AppendItem(rootBranch, "Bookmarks");
    recentBranch = m_treeView->AppendItem(rootBranch, "Recents");
    
    expandState["active"] = true;
    expandState["range"] = false;
    expandState["bookmark"] = true;
    expandState["recent"] = true;
    
    doUpdateActive.store(true);
    doUpdateBookmarks.store(true);
    bookmarkChoice = nullptr;
    dragItem = nullptr;
    dragItemId = nullptr;
    editingLabel = false;
    
    m_clearSearchButton->Hide();
    hideProps();
    
    m_updateTimer.Start(500);
   
    visualDragItem = nullptr;
    nextEnt = nullptr;
    nextDemod = nullptr;
    nextGroup = "";

    mouseTracker.setTarget(this);
}

BookmarkView::~BookmarkView() {

    dragItem = nullptr;
    dragItemId = nullptr;
    editingLabel = false;

    visualDragItem = nullptr;
    nextEnt = nullptr;
    nextDemod = nullptr;
}

void BookmarkView::onUpdateTimer( wxTimerEvent& /* event */ ) {
    if (!this->IsShown()) {
        return;
    }
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
    
    SetBackgroundColour(bgColor);

    m_treeView->SetBackgroundColour(bgColor);
    m_treeView->SetForegroundColour(textColor);
    
    m_propPanel->SetBackgroundColour(bgColor);
    m_propPanel->SetForegroundColour(textColor);

    m_buttonPanel->SetBackgroundColour(bgColor);
    m_buttonPanel->SetForegroundColour(textColor);
    
    m_labelLabel->SetForegroundColour(textColor);
    m_frequencyVal->SetForegroundColour(textColor);
    m_frequencyLabel->SetForegroundColour(textColor);
    m_bandwidthVal->SetForegroundColour(textColor);
    m_bandwidthLabel->SetForegroundColour(textColor);
    m_modulationVal->SetForegroundColour(textColor);
    m_modulationLabel->SetForegroundColour(textColor);
    
    refreshLayout();
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

bool BookmarkView::isKeywordMatch(std::wstring search_str, std::vector<std::wstring> &keywords) {
    wstring str = search_str;
    std::transform(str.begin(), str.end(), str.begin(), towlower);

    for (auto k : keywords) {
        if (str.find(k) == wstring::npos) {
            return false;
        }
    }
    
    return true;
}

wxTreeItemId BookmarkView::refreshBookmarks() {
    
    //capture the previously selected item info BY COPY (because the original will be destroyed together with the destroyed tree items) to restore it again after 
    //having rebuilding the whole tree.
    TreeViewItem* prevSel = itemToTVI(m_treeView->GetSelection());
    TreeViewItem* prevSelCopy = nullptr;

    if (prevSel != NULL) {
        prevSelCopy = new TreeViewItem(*prevSel);
    }

    BookmarkNames groupNames;
    wxGetApp().getBookmarkMgr().getGroups(groupNames);
    
    doUpdateBookmarkGroup.clear();
   
    wxTreeItemId bmSelFound = nullptr;
    
    std::map<std::string, bool> groupExpandState;
    bool searchState = (searchKeywords.size() != 0);
    
    groups.clear();

    m_treeView->DeleteChildren(bookmarkBranch);

    bool bmExpandState = expandState["bookmark"];

    for (auto gn_i : groupNames) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP;
        tvi->groupName = gn_i;
        wxTreeItemId group_itm = m_treeView->AppendItem(bookmarkBranch, gn_i);
        SetTreeItemData(group_itm, tvi);
        groups[gn_i] = group_itm;
        if (prevSelCopy != nullptr && prevSelCopy->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP && gn_i == prevSelCopy->groupName) {
            bmSelFound = group_itm;
        } else if (nextGroup != "" && gn_i == nextGroup) {
            bmSelFound = group_itm;
            nextGroup = "";
        }
    }

    if (searchState || bmExpandState) {
        m_treeView->Expand(bookmarkBranch);
    } else {
        m_treeView->Collapse(bookmarkBranch);
    }

    for (auto gn_i : groupNames) {
        wxTreeItemId groupItem = groups[gn_i];

        bool groupExpanded = searchState || wxGetApp().getBookmarkMgr().getExpandState(gn_i);

        const BookmarkList& bmList = wxGetApp().getBookmarkMgr().getBookmarks(gn_i);

        for (auto &bmEnt : bmList) {
            std::wstring labelVal = BookmarkMgr::getBookmarkEntryDisplayName(bmEnt);

            if (searchState) {
                std::string freqStr = frequencyToStr(bmEnt->frequency);
                std::string bwStr = frequencyToStr(bmEnt->bandwidth);

                std::wstring fullText = labelVal +
                    L" " + bmEnt->label +
                    L" " + std::to_wstring(bmEnt->frequency) +
                    L" " + std::wstring(freqStr.begin(),freqStr.end()) +
                    L" " + std::wstring(bwStr.begin(),bwStr.end()) +
                    L" " + std::wstring(bmEnt->type.begin(),bmEnt->type.end());
                
                if (!isKeywordMatch(fullText, searchKeywords)) {
                    continue;
                }
            }
            
            TreeViewItem* tvi = new TreeViewItem();
            tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK;
            tvi->bookmarkEnt = bmEnt;
            tvi->groupName = gn_i;
            
            wxTreeItemId itm = m_treeView->AppendItem(groupItem, labelVal);
            SetTreeItemData(itm, tvi);
            if (prevSelCopy != nullptr && prevSelCopy->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK && prevSelCopy->bookmarkEnt == bmEnt && groupExpanded) {
                bmSelFound = itm;
            }
            if (nextEnt == bmEnt) {
                bmSelFound = itm;
                nextEnt = nullptr;
            }
        }

        if (groupExpanded) {
            m_treeView->Expand(groupItem);
        }
    }

    delete prevSelCopy;

    return bmSelFound;
}


void BookmarkView::doUpdateActiveList() {

    auto demods = wxGetApp().getDemodMgr().getDemodulators();
    auto lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    //capture the previously selected item info BY COPY (because the original will be destroyed together with the destroyed tree items) to restore it again after 
    //having rebuilding the whole tree.
    TreeViewItem* prevSel = itemToTVI(m_treeView->GetSelection());
    TreeViewItem* prevSelCopy = nullptr;
   
    if (prevSel != NULL) {
        prevSelCopy = new TreeViewItem(*prevSel);
    }

    // Actives
    m_treeView->DeleteChildren(activeBranch);
    
    bool activeExpandState = expandState["active"];
    bool searchState = (searchKeywords.size() != 0);
    
    wxTreeItemId selItem = nullptr;
    for (auto demod_i : demods) {
        wxString activeLabel = BookmarkMgr::getActiveDisplayName(demod_i);
        
        if (searchState) {
            std::string freqStr = frequencyToStr(demod_i->getFrequency());
            std::string bwStr = frequencyToStr(demod_i->getBandwidth());
            std::string mtype = demod_i->getDemodulatorType();
            
            std::wstring fullText = activeLabel.ToStdWstring() +
            L" " + demod_i->getDemodulatorUserLabel() +
            L" " + std::to_wstring(demod_i->getFrequency()) +
            L" " + std::wstring(freqStr.begin(),freqStr.end()) +
            L" " + std::wstring(bwStr.begin(),bwStr.end()) +
            L" " + std::wstring(mtype.begin(),mtype.end());
            
            if (!isKeywordMatch(fullText, searchKeywords)) {
                continue;
            }
        }

        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE;
        tvi->demod = demod_i;

        wxTreeItemId itm = m_treeView->AppendItem(activeBranch,activeLabel);
        SetTreeItemData(itm, tvi);
        
        if (nextDemod != nullptr && nextDemod == demod_i) {
            selItem = itm;
            wxGetApp().getDemodMgr().setActiveDemodulator(nextDemod, false);
            nextDemod = nullptr;
        } else if (!selItem && activeExpandState && lastActiveDemodulator && lastActiveDemodulator == demod_i) {
            selItem = itm;
        }
    }

    bool rangeExpandState = searchState?false:expandState["range"];
    
	//Ranges
    const BookmarkRangeList& bmRanges = wxGetApp().getBookmarkMgr().getRanges();

    m_treeView->DeleteChildren(rangeBranch);
    
    for (auto &re_i: bmRanges) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_RANGE;
        tvi->rangeEnt = re_i;
        
        std::wstring labelVal = re_i->label;
        
        if (labelVal == "") {
            std::string wstr = frequencyToStr(re_i->startFreq) + " - " + frequencyToStr(re_i->endFreq);
            labelVal = std::wstring(wstr.begin(),wstr.end());
        }
        
        wxTreeItemId itm = m_treeView->AppendItem(rangeBranch, labelVal);
        SetTreeItemData(itm, tvi);
        
        if (nextRange == re_i) {
            selItem = itm;
            nextRange = nullptr;
        } else if (!selItem && rangeExpandState && prevSelCopy && prevSelCopy->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RANGE && prevSelCopy->rangeEnt == re_i) {
            selItem = itm;
        }
    }
     
    bool recentExpandState = searchState || expandState["recent"];
    
    // Recents
    const BookmarkList& bmRecents = wxGetApp().getBookmarkMgr().getRecents();
    m_treeView->DeleteChildren(recentBranch);
    
    for (auto &bmr_i: bmRecents) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT;
        tvi->bookmarkEnt = bmr_i;

        std::wstring labelVal;
        bmr_i->node->child("user_label")->element()->get(labelVal);

        if (labelVal == "") {
            std::string wstr = frequencyToStr(bmr_i->frequency) + " " + bmr_i->type;
            labelVal = std::wstring(wstr.begin(),wstr.end());
        }
        
        if (searchKeywords.size()) {
            
            std::string freqStr = frequencyToStr(bmr_i->frequency);
            std::string bwStr = frequencyToStr(bmr_i->bandwidth);
            
            std::wstring fullText = labelVal +
                L" " + std::to_wstring(bmr_i->frequency) +
                L" " + std::wstring(freqStr.begin(),freqStr.end()) +
                L" " + std::wstring(bwStr.begin(),bwStr.end()) +
                L" " + std::wstring(bmr_i->type.begin(),tvi->bookmarkEnt->type.end());
            
            if (!isKeywordMatch(fullText, searchKeywords)) {
                continue;
            }
        }
        
        wxTreeItemId itm = m_treeView->AppendItem(recentBranch, labelVal);
        SetTreeItemData(itm, tvi);

        if (nextEnt == bmr_i) {
            selItem = itm;
            nextEnt = nullptr;
        } else if (!selItem && recentExpandState && prevSelCopy && prevSelCopy->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT && prevSelCopy->bookmarkEnt == bmr_i) {
            selItem = itm;
        }
    }
    
    if (activeExpandState) {
        m_treeView->Expand(activeBranch);
    } else {
        m_treeView->Collapse(activeBranch);
    }
    if (recentExpandState) {
        m_treeView->Expand(recentBranch);
    } else {
        m_treeView->Collapse(recentBranch);
    }
    if (rangeExpandState) {
        m_treeView->Expand(rangeBranch);
    } else {
        m_treeView->Collapse(rangeBranch);
    }

    //select the item having the same meaning as the previously selected item
    if (selItem != nullptr) {
        m_treeView->SelectItem(selItem);
    }

    delete prevSelCopy;
}


void BookmarkView::onTreeActivate( wxTreeEvent& event ) {

    wxTreeItemId itm = event.GetItem();
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(itm));

    if (tvi) {
        if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
            if (!tvi->demod->isActive()) {                
                wxGetApp().setFrequency(tvi->demod->getFrequency());
                nextDemod = tvi->demod;
            }
        } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
             
            nextEnt = tvi->bookmarkEnt;
            wxGetApp().getBookmarkMgr().removeRecent(tvi->bookmarkEnt);

            activateBookmark(tvi->bookmarkEnt);
        } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
            activateBookmark(tvi->bookmarkEnt);
        } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RANGE) {
            activateRange(tvi->rangeEnt);
        }
    }
}


void BookmarkView::onTreeCollapse( wxTreeEvent& event ) {
    bool searchState = (searchKeywords.size() != 0);
    
    if (searchState) {
        event.Skip();
        return;
    }
    
    if (event.GetItem() == activeBranch) {
        expandState["active"] = false;
    } else if (event.GetItem() == bookmarkBranch) {
        expandState["bookmark"] = false;
    } else if (event.GetItem() == recentBranch) {
        expandState["recent"] = false;
    } else if (event.GetItem() == rangeBranch) {
        expandState["range"] = false;
    } else {
        TreeViewItem *tvi = itemToTVI(event.GetItem());
        
        if (tvi != nullptr) {
            if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
                wxGetApp().getBookmarkMgr().setExpandState(tvi->groupName,false);
            }
        }

    }
}


void BookmarkView::onTreeExpanded( wxTreeEvent& event ) {
    bool searchState = (searchKeywords.size() != 0);
    
    if (searchState) {
        event.Skip();
        return;
    }
    
    if (event.GetItem() == activeBranch) {
        expandState["active"] = true;
    } else if (event.GetItem() == bookmarkBranch) {
        expandState["bookmark"] = true;
    } else if (event.GetItem() == recentBranch) {
        expandState["recent"] = true;
    } else if (event.GetItem() == rangeBranch) {
        expandState["range"] = true;
    } else {
        TreeViewItem *tvi = itemToTVI(event.GetItem());
        
        if (tvi != nullptr) {
            if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
                wxGetApp().getBookmarkMgr().setExpandState(tvi->groupName,true);
            }
        }
        
    }
}


void BookmarkView::onTreeItemMenu( wxTreeEvent& /* event */ ) {
    if (m_treeView->GetSelection() == bookmarkBranch) {
        wxMenu menu;
        menu.Append(wxCONTEXT_ADD_GROUP_ID, BOOKMARK_VIEW_STR_ADD_GROUP);
        menu.Connect(wxCONTEXT_ADD_GROUP_ID, wxEVT_MENU, (wxObjectEventFunction)&BookmarkView::onMenuItem, nullptr, this);
        PopupMenu(&menu);
    }
}


void BookmarkView::onMenuItem(wxCommandEvent& event) {
    if (event.GetId() == wxCONTEXT_ADD_GROUP_ID) {
        onAddGroup(event);
    }
}


bool BookmarkView::isMouseInView() {
    if (editingLabel) {
        return true;
    }
    if (m_labelText->HasFocus()) {
        return true;
    }
    if (m_searchText->HasFocus()) {
        return true;
    }
    return  mouseTracker.mouseInView();
}


bool BookmarkView::getExpandState(std::string branchName) {
    return expandState[branchName];
}


void BookmarkView::setExpandState(std::string branchName, bool state) {
    expandState[branchName] = state;
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
    
    m_propPanel->Hide();
    m_buttonPanel->Hide();
}


void BookmarkView::showProps() {
    m_propPanel->Show();
    m_propPanel->GetSizer()->Layout();
}


void BookmarkView::clearButtons() {
    m_buttonPanel->Hide();
    m_buttonPanel->DestroyChildren();
    bookmarkChoice = nullptr;
}

void BookmarkView::showButtons() {
    m_buttonPanel->Show();
    m_buttonPanel->GetSizer()->Layout();
}

void BookmarkView::refreshLayout() {
    GetSizer()->Layout();
    Update();
    Refresh();
}


wxButton *BookmarkView::makeButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler) {
    wxButton *nButton = new wxButton( m_buttonPanel, wxID_ANY, labelVal);
    nButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, handler, nullptr, this);
    
    wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
    wxColour textColor(ThemeMgr::mgr.currentTheme->text);

    nButton->SetBackgroundColour(bgColor);
    nButton->SetForegroundColour(textColor);
    return nButton;
}


wxButton *BookmarkView::addButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler) {
    wxButton *nButton = makeButton(parent, labelVal, handler);
    parent->GetSizer()->Add( nButton, 0, wxEXPAND);
    return nButton;
}


void BookmarkView::doBookmarkActive(std::string group, DemodulatorInstancePtr demod) {

    wxGetApp().getBookmarkMgr().addBookmark(group, demod);
    wxGetApp().getBookmarkMgr().updateBookmarks();
}


void BookmarkView::doBookmarkRecent(std::string group, BookmarkEntryPtr be) {
    
    wxGetApp().getBookmarkMgr().addBookmark(group, be);
    nextEnt = be;
    wxGetApp().getBookmarkMgr().removeRecent(be);
    wxGetApp().getBookmarkMgr().updateBookmarks();
    bookmarkSelection(be);
}


void BookmarkView::doMoveBookmark(BookmarkEntryPtr be, std::string group) {
    wxGetApp().getBookmarkMgr().moveBookmark(be, group);
    nextEnt = be;
    wxGetApp().getBookmarkMgr().updateBookmarks();
    bookmarkSelection(be);
}


void BookmarkView::doRemoveActive(DemodulatorInstancePtr demod) {

	wxGetApp().getBookmarkMgr().removeActive(demod);
	wxGetApp().getBookmarkMgr().updateActiveList();
}


void BookmarkView::doRemoveRecent(BookmarkEntryPtr be) {
    wxGetApp().getBookmarkMgr().removeRecent(be);
    wxGetApp().getBookmarkMgr().updateActiveList();
}

void BookmarkView::doClearRecents() {
    wxGetApp().getBookmarkMgr().clearRecents();
    wxGetApp().getBookmarkMgr().updateActiveList();
}


void BookmarkView::updateBookmarkChoices() {

    bookmarkChoices.clear();

    TreeViewItem *activeSel = itemToTVI(m_treeView->GetSelection());
    
    bookmarkChoices.push_back(((activeSel != nullptr && activeSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK))?BOOKMARK_VIEW_CHOICE_MOVE:BOOKMARK_VIEW_CHOICE_DEFAULT);
    wxGetApp().getBookmarkMgr().getGroups(bookmarkChoices);
    bookmarkChoices.push_back(BOOKMARK_VIEW_CHOICE_NEW);
}

void BookmarkView::addBookmarkChoice(wxWindow *parent) {
    updateBookmarkChoices();
    bookmarkChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, bookmarkChoices, wxEXPAND, wxDefaultValidator, "Bookmark");
    bookmarkChoice->Select(0);
    bookmarkChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, (wxObjectEventFunction)&BookmarkView::onBookmarkChoice, nullptr, this);
    parent->GetSizer()->Add(bookmarkChoice, 0, wxALL | wxEXPAND);
}


void BookmarkView::onBookmarkChoice( wxCommandEvent & /* event */ ) {

    TreeViewItem *tvi = itemToTVI(m_treeView->GetSelection());
    
    int numSel = bookmarkChoice->GetCount();
    int sel = bookmarkChoice->GetSelection();
    
    if (sel == 0) {
        return;
    }
    
    wxString stringVal = "";
    
    if (sel == (numSel-1)) {
        stringVal = wxGetTextFromUser(BOOKMARK_VIEW_STR_ADD_GROUP_DESC, BOOKMARK_VIEW_STR_ADD_GROUP, "");
    } else {
        stringVal = bookmarkChoices[sel];
    }
    
    if (stringVal == "") {
        return;
    }

    if (tvi != nullptr) {
        if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
            doBookmarkActive(stringVal.ToStdString(), tvi->demod);
        }
        if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
            doBookmarkRecent(stringVal.ToStdString(), tvi->bookmarkEnt);
        }
        if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
            doMoveBookmark(tvi->bookmarkEnt, stringVal.ToStdString());
        }
    }
}


void BookmarkView::activeSelection(DemodulatorInstancePtr dsel) {
    
    m_frequencyVal->SetLabelText(frequencyToStr(dsel->getFrequency()));
    m_bandwidthVal->SetLabelText(frequencyToStr(dsel->getBandwidth()));
    m_modulationVal->SetLabelText(dsel->getDemodulatorType());
    m_labelText->SetValue(dsel->getDemodulatorUserLabel());
    
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

    addBookmarkChoice(m_buttonPanel);
    addButton(m_buttonPanel, "Remove Active", wxCommandEventHandler( BookmarkView::onRemoveActive ));

    showProps();
    showButtons();
    refreshLayout();
}


void BookmarkView::activateBookmark(BookmarkEntryPtr bmEnt) {

	wxTreeItemId selItem = m_treeView->GetSelection();
	if (selItem) {
		m_treeView->SelectItem(selItem, false);
	}

	//if a matching DemodulatorInstance do not exist yet, create it and activate it, else use
	//the already existing one:
	// we search among the list of existing demodulators the one matching 
	//bmEnt and activate it. The search is made backwards, to select the most recently created one.
	DemodulatorInstancePtr matchingDemod = wxGetApp().getDemodMgr().getLastDemodulatorWith(
																		bmEnt->type,
																		bmEnt->label, 
																		bmEnt->frequency, 
																		bmEnt->bandwidth);
	//not found, create a new demod instance: 
	if (matchingDemod == nullptr) {

		matchingDemod = wxGetApp().getDemodMgr().loadInstance(bmEnt->node);
		matchingDemod->run();
		wxGetApp().notifyDemodulatorsChanged();
	}

	matchingDemod->setActive(true);

	long long freq = matchingDemod->getFrequency();
	long long currentFreq = wxGetApp().getFrequency();
	long long currentRate = wxGetApp().getSampleRate();

	if ((abs(freq - currentFreq) > currentRate / 2) || (abs(currentFreq - freq) > currentRate / 2)) {
		wxGetApp().setFrequency(freq);
	}

	nextDemod = matchingDemod;
  
	wxGetApp().getBookmarkMgr().updateActiveList();
}


void BookmarkView::activateRange(BookmarkRangeEntryPtr rangeEnt) {
    wxGetApp().setFrequency(rangeEnt->freq);
    wxGetApp().getAppFrame()->setViewState(rangeEnt->startFreq + (rangeEnt->endFreq - rangeEnt->startFreq) / 2, rangeEnt->endFreq - rangeEnt->startFreq);
}


void BookmarkView::bookmarkSelection(BookmarkEntryPtr bmSel) {
    
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
    
    addBookmarkChoice(m_buttonPanel);
    addButton(m_buttonPanel, "Activate Bookmark", wxCommandEventHandler( BookmarkView::onActivateBookmark ));
    addButton(m_buttonPanel, "Remove Bookmark", wxCommandEventHandler( BookmarkView::onRemoveBookmark ));
   
    showProps();
    showButtons();
    refreshLayout();
}


void BookmarkView::recentSelection(BookmarkEntryPtr bmSel) {
    
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
    
    addBookmarkChoice(m_buttonPanel);
    addButton(m_buttonPanel, "Activate Recent", wxCommandEventHandler( BookmarkView::onActivateRecent ));
    addButton(m_buttonPanel, "Remove Recent", wxCommandEventHandler( BookmarkView::onRemoveRecent ));
    
    showProps();
    showButtons();
    refreshLayout();
}

void BookmarkView::groupSelection(std::string groupName) {
    
    clearButtons();
    
    hideProps();
    
    m_labelText->SetValue(groupName);
    
    m_labelText->Show();
    m_labelLabel->Show();
    
    addButton(m_buttonPanel, "Remove Group", wxCommandEventHandler( BookmarkView::onRemoveGroup ));
    
    showProps();
    
    showButtons();
    refreshLayout();
}


void BookmarkView::rangeSelection(BookmarkRangeEntryPtr re) {
    
    clearButtons();
    
    hideProps();
    
    m_labelText->SetValue(re->label);
    
    m_labelText->Show();
    m_labelLabel->Show();

    m_frequencyVal->Show();
    m_frequencyLabel->Show();

    std::string strFreq = frequencyToStr(re->startFreq) + "-" + frequencyToStr(re->endFreq);
    
    m_frequencyVal->SetLabelText(std::wstring(strFreq.begin(),strFreq.end()));
    
    showProps();

    addButton(m_buttonPanel, "Go to Range", wxCommandEventHandler( BookmarkView::onActivateRange ));
    addButton(m_buttonPanel, "Update Range", wxCommandEventHandler( BookmarkView::onUpdateRange ))->SetToolTip("Update range by setting it to the active range.");
    addButton(m_buttonPanel, "Remove Range", wxCommandEventHandler( BookmarkView::onRemoveRange ));
    
    showButtons();
    refreshLayout();
}


void BookmarkView::bookmarkBranchSelection() {
    
    clearButtons();
    hideProps();
    
    addButton(m_buttonPanel, BOOKMARK_VIEW_STR_ADD_GROUP, wxCommandEventHandler( BookmarkView::onAddGroup ));
    
    showButtons();
    refreshLayout();
}


void BookmarkView::recentBranchSelection() {
    clearButtons();
    hideProps();
    
    addButton(m_buttonPanel, BOOKMARK_VIEW_STR_CLEAR_RECENT, wxCommandEventHandler( BookmarkView::onClearRecents ));
    
    showButtons();
    refreshLayout();

    this->Layout();
}


void BookmarkView::rangeBranchSelection() {
    clearButtons();
    hideProps();
    
    m_labelText->SetValue(wxT(""));
    m_labelText->Show();
    m_labelLabel->Show();

    showProps();

    addButton(m_buttonPanel, "Add Active Range", wxCommandEventHandler( BookmarkView::onAddRange ));
    
    showButtons();
    refreshLayout();
    
    this->Layout();
}


void BookmarkView::activeBranchSelection() {
    hideProps();
    this->Layout();
}


void BookmarkView::onTreeSelect( wxTreeEvent& event ) {
    wxTreeItemId itm = event.GetItem();
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(itm));

    if (!tvi) {
        
        if (itm == bookmarkBranch) {
            bookmarkBranchSelection();
        } else if (itm == activeBranch) {
            activeBranchSelection();
        } else if (itm == recentBranch) {
            recentBranchSelection();
        } else if (itm == rangeBranch) {
            rangeBranchSelection();
        } else {
            hideProps();
            this->Layout();
        }
        
        return;
    }
                                                    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        activeSelection(tvi->demod);
        if (tvi->demod->isActive()) {
            wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, true);
            wxGetApp().getDemodMgr().setActiveDemodulator(tvi->demod, false);
            tvi->demod->setTracking(true);
        }
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        recentSelection(tvi->bookmarkEnt);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        bookmarkSelection(tvi->bookmarkEnt);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        groupSelection(tvi->groupName);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RANGE) {
        rangeSelection(tvi->rangeEnt);
    } else {
        hideProps();
        this->Layout();
    }
}


void BookmarkView::onTreeSelectChanging( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onLabelText( wxCommandEvent& /* event */ ) {
    std::wstring newLabel = m_labelText->GetValue().ToStdWstring();
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel != nullptr) {
        if (curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
            curSel->demod->setDemodulatorUserLabel(newLabel);
            wxGetApp().getBookmarkMgr().updateActiveList();
        } else if (curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
            curSel->bookmarkEnt->label = m_labelText->GetValue().ToStdWstring();
            curSel->bookmarkEnt->node->child("user_label")->element()->set(newLabel);
            wxGetApp().getBookmarkMgr().updateBookmarks();
        } else if (curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
            curSel->bookmarkEnt->label = m_labelText->GetValue().ToStdWstring();
            curSel->bookmarkEnt->node->child("user_label")->element()->set(newLabel);
            wxGetApp().getBookmarkMgr().updateActiveList();
        } else if (curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RANGE) {
            curSel->rangeEnt->label = m_labelText->GetValue().ToStdWstring();
            wxGetApp().getBookmarkMgr().updateActiveList();
        } else if (curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
            std::string newGroupName = m_labelText->GetValue().ToStdString();

            if (newGroupName != "" && newGroupName != curSel->groupName) {
                wxGetApp().getBookmarkMgr().renameGroup(curSel->groupName, newGroupName);
                nextGroup = newGroupName;
                wxGetApp().getBookmarkMgr().updateBookmarks();
            }
        }
    }
}


void BookmarkView::onDoubleClickFreq( wxMouseEvent& /* event */ ) {

    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, true);
        wxGetApp().getDemodMgr().setActiveDemodulator(curSel->demod, false);
        wxGetApp().showFrequencyInput(FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_DEFAULT);
    }
}


void BookmarkView::onDoubleClickBandwidth( wxMouseEvent& /* event */ ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, true);
        wxGetApp().getDemodMgr().setActiveDemodulator(curSel->demod, false);
        wxGetApp().showFrequencyInput(FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_BANDWIDTH);
    }
}


void BookmarkView::onRemoveActive( wxCommandEvent& /* event */ ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        if (editingLabel) {
            return;
        }
        doRemoveActive(curSel->demod);
    }
}


void BookmarkView::onRemoveBookmark( wxCommandEvent& /* event */ ) {
    if (editingLabel) {
        return;
    }

    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        ActionDialog::showDialog(new ActionDialogRemoveBookmark(curSel->bookmarkEnt));
    }
}


void BookmarkView::onActivateBookmark( wxCommandEvent& /* event */ ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        activateBookmark(curSel->bookmarkEnt);
    }
}


void BookmarkView::onActivateRecent( wxCommandEvent& /* event */ ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        BookmarkEntryPtr bookmarkEntToActivate = curSel->bookmarkEnt;
      
        //let removeRecent() + activateBookmark() refresh the tree 
        //and delete the recent node properly...
        wxGetApp().getBookmarkMgr().removeRecent(bookmarkEntToActivate);

        activateBookmark(bookmarkEntToActivate);   
    }
}


void BookmarkView::onRemoveRecent ( wxCommandEvent& /* event */ ) {
    if (editingLabel) {
        return;
    }
    
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        BookmarkEntryPtr bookmarkEntToRemove = curSel->bookmarkEnt;
        
        //let removeRecent() + updateActiveList() refresh the tree 
        //and delete the recent node properly...
        wxGetApp().getBookmarkMgr().removeRecent(bookmarkEntToRemove);
        wxGetApp().getBookmarkMgr().updateActiveList();
    }
}

void BookmarkView::onClearRecents ( wxCommandEvent& /* event */ ) {
    if (editingLabel) {
        return;
    }
    doClearRecents();
}


void BookmarkView::onAddGroup( wxCommandEvent& /* event */ ) {
    wxString stringVal = wxGetTextFromUser(BOOKMARK_VIEW_STR_ADD_GROUP_DESC, BOOKMARK_VIEW_STR_ADD_GROUP, "");
    if (stringVal.ToStdString() != "") {
        wxGetApp().getBookmarkMgr().addGroup(stringVal.ToStdString());
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
}


void BookmarkView::onRemoveGroup( wxCommandEvent& /* event */ ) {
    if (editingLabel) {
        return;
    }

    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        ActionDialog::showDialog(new ActionDialogRemoveGroup(curSel->groupName));
    }
}


void BookmarkView::onAddRange( wxCommandEvent& /* event */ ) {
    
    BookmarkRangeEntryPtr re = BookmarkView::makeActiveRangeEntry();
    
    re->label = m_labelText->GetValue();

    wxGetApp().getBookmarkMgr().addRange(re);
    wxGetApp().getBookmarkMgr().updateActiveList();
}


void BookmarkView::onRemoveRange( wxCommandEvent& /* event */ ) {
    if (editingLabel) {
        return;
    }
    
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RANGE) {
        ActionDialog::showDialog(new ActionDialogRemoveRange(curSel->rangeEnt));
    }
}


void BookmarkView::onRenameRange( wxCommandEvent& /* event */ ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (!curSel || curSel->type != TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        return;
    }
    
    wxString stringVal = "";
    stringVal = wxGetTextFromUser(BOOKMARK_VIEW_STR_RENAME_GROUP, "New Group Name", curSel->groupName);
    
    std::string newGroupName = stringVal.Trim().ToStdString();
    
    if (newGroupName != "") {
        wxGetApp().getBookmarkMgr().renameGroup(curSel->groupName, newGroupName);
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
}

void BookmarkView::onActivateRange( wxCommandEvent& /* event */ ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RANGE) {
        activateRange(curSel->rangeEnt);
    }
}

void BookmarkView::onUpdateRange( wxCommandEvent& /* event */ ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RANGE) {
        ActionDialog::showDialog(new ActionDialogUpdateRange(curSel->rangeEnt));
    }
}

void BookmarkView::onTreeBeginDrag( wxTreeEvent& event ) {
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));
    
    dragItem = nullptr;
    dragItemId = nullptr;

    SetCursor(wxCURSOR_CROSS);

    if (!tvi) {
        event.Veto();
        return;
    }
    
    bool bAllow = false;
    std::wstring dragItemName;
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        bAllow = true;
        dragItemName = BookmarkMgr::getActiveDisplayName(tvi->demod);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT || tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        bAllow = true;
        dragItemName = BookmarkMgr::getBookmarkEntryDisplayName(tvi->bookmarkEnt);
    }
    
    if (bAllow) {
        wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
        wxColour textColor(ThemeMgr::mgr.currentTheme->text);
        
        m_treeView->SetBackgroundColour(textColor);
        m_treeView->SetForegroundColour(bgColor);
//        m_treeView->SetToolTip("Dragging " + dragItemName);
        
        dragItem = tvi;
        dragItemId = event.GetItem();

        visualDragItem = new BookmarkViewVisualDragItem(dragItemName);
        
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

    SetCursor(wxCURSOR_ARROW);

    if (visualDragItem != nullptr) {
        visualDragItem->Destroy();
        delete visualDragItem;
        visualDragItem = nullptr;
    }
    
    if (!event.GetItem()) {
        event.Veto();
        return;
    }

    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));

    if (!tvi) {
        if (event.GetItem() == bookmarkBranch) {
            if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
                doBookmarkActive(BOOKMARK_VIEW_STR_UNNAMED, dragItem->demod);
            } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
                doBookmarkRecent(BOOKMARK_VIEW_STR_UNNAMED, dragItem->bookmarkEnt);
            } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
                doMoveBookmark(dragItem->bookmarkEnt, BOOKMARK_VIEW_STR_UNNAMED);
            }
        }
        return;
    }
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) { // Active -> Group Item
            doBookmarkActive(tvi->groupName, dragItem->demod);
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) { // Recent -> Group Item
            doBookmarkRecent(tvi->groupName, dragItem->bookmarkEnt);
            m_treeView->Delete(dragItemId);
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) { // Bookmark -> Group Item
            doMoveBookmark(dragItem->bookmarkEnt, tvi->groupName);
        }
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) { // Active -> Same Group
            doBookmarkActive(tvi->groupName, dragItem->demod);
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) { // Recent -> Same Group
            doBookmarkRecent(tvi->groupName, dragItem->bookmarkEnt);
            m_treeView->Delete(dragItemId);
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) { // Bookmark -> Same Group
            doMoveBookmark(dragItem->bookmarkEnt, tvi->groupName);
        }
    }
}


void BookmarkView::onTreeItemGetTooltip( wxTreeEvent& event ) {
    
    event.Skip();
}


void BookmarkView::onEnterWindow( wxMouseEvent&  event ) {
    mouseTracker.OnMouseEnterWindow(event);

#ifdef _WIN32
    if (wxGetApp().getAppFrame()->canFocus()) {
        //make mousewheel work in the tree view.
        m_treeView->SetFocus();
    }
#endif

    setStatusText("Drag & Drop to create / move bookmarks, Group and arrange bookmarks, quick Search by keywords.");
}


void BookmarkView::onLeaveWindow( wxMouseEvent& event ) {
    mouseTracker.OnMouseLeftWindow(event);
}

void BookmarkView::onMotion( wxMouseEvent& event ) {
    mouseTracker.OnMouseMoved(event);

    wxPoint pos = ClientToScreen(event.GetPosition());
    
    pos += wxPoint(30,-10);
    
    if (visualDragItem != nullptr) {
        visualDragItem->SetPosition(pos);
    }
   
    event.Skip();
}

void BookmarkView::setStatusText(std::string statusText) {
    //make tooltips active on the tree view.
    wxGetApp().getAppFrame()->setStatusText(m_treeView, statusText);
}

TreeViewItem *BookmarkView::itemToTVI(wxTreeItemId item) {
    TreeViewItem* tvi = nullptr;
    
    if (item != nullptr) {
        tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(item));
    }
    
    return tvi;
}

void BookmarkView::onSearchTextFocus( wxMouseEvent&  event ) {
    mouseTracker.OnMouseMoved(event);
    
    //apparently needed ???
    m_searchText->SetFocus();

    if (m_searchText->GetValue() == L"Search..") {
        //select the whole field, so that typing 
        //replaces the whole text by the new one right away.  
        m_searchText->SetSelection(-1, -1);
    }
    else if (!m_searchText->GetValue().Trim().empty()) {
        //position at the end of the existing field, so we can append 
        //or truncate the existing field.
        m_searchText->SetInsertionPointEnd();
    }
    else {
        //empty field, restore displaying L"Search.."
        m_searchText->SetValue(L"Search..");
        m_searchText->SetSelection(-1, -1);
    }
}


void BookmarkView::onSearchText( wxCommandEvent& event ) {
    wstring searchText = m_searchText->GetValue().Trim().Lower().ToStdWstring();
    
   searchKeywords.clear();
    
   if (searchText.length() != 0) {
        std::wstringstream searchTextLo(searchText);
        wstring tmp;
        
        while(std::getline(searchTextLo, tmp, L' ')) {
            if (tmp.length() != 0 && tmp.find(L"search.") == wstring::npos) {
                searchKeywords.push_back(tmp);
//                std::wcout << L"Keyword: " << tmp << '\n';
            }
        }
    }
    
    if (searchKeywords.size() != 0 && !m_clearSearchButton->IsShown()) {
        m_clearSearchButton->Show();
        refreshLayout();
    } else if (searchKeywords.size() == 0 && m_clearSearchButton->IsShown()) {
        m_clearSearchButton->Hide();
        refreshLayout();
    }
    
    wxGetApp().getBookmarkMgr().updateActiveList();
    wxGetApp().getBookmarkMgr().updateBookmarks();
}


void BookmarkView::onClearSearch( wxCommandEvent& /* event */ ) {
    m_clearSearchButton->Hide();
    m_searchText->SetValue(L"Search..");
    m_treeView->SetFocus();

    searchKeywords.clear();

    wxGetApp().getBookmarkMgr().updateActiveList();
    wxGetApp().getBookmarkMgr().updateBookmarks();
    refreshLayout();
}

void BookmarkView::loadDefaultRanges() {

    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"160 Meters", 1900000, 1800000, 2000000));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"80 Meters", 3750000, 3500000, 4000000));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"60 Meters", 5368500, 5332000, 5405000));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"40 Meters", 7150000, 7000000, 7300000));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"30 Meters", 10125000, 10100000, 10150000));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"20 Meters", 14175000, 14000000, 14350000));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"17 Meters", 18068180, 17044180, 19092180));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"15 Meters", 21225000, 21000000, 21450000));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"12 Meters", 24940000, 24890000, 24990000));
    wxGetApp().getBookmarkMgr().addRange(std::make_shared<BookmarkRangeEntry>(L"10 Meters", 28850000, 28000000, 29700000));
}


BookmarkRangeEntryPtr BookmarkView::makeActiveRangeEntry() {
    BookmarkRangeEntryPtr re(new BookmarkRangeEntry);
    re->freq = wxGetApp().getFrequency();
    re->startFreq = wxGetApp().getAppFrame()->getViewCenterFreq() - (wxGetApp().getAppFrame()->getViewBandwidth()/2);
    re->endFreq = wxGetApp().getAppFrame()->getViewCenterFreq() + (wxGetApp().getAppFrame()->getViewBandwidth()/2);
    
    return re;
}


void BookmarkView::SetTreeItemData(const wxTreeItemId& item, wxTreeItemData *data) {

    TreeViewItem *itemData = itemToTVI(item);
    if (itemData != NULL) {
         //cleanup previous data, if any
        delete itemData;
    }

    m_treeView->SetItemData(item, data);
}
