#include "BookmarkView.h"
#include "CubicSDR.h"

BookmarkView::BookmarkView( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : BookmarkPanel(parent, id, pos, size, style) {

    rootBranch = m_treeView->AddRoot("Root");
    activeBranch = m_treeView->AppendItem(rootBranch, "Active");
    bookmarkBranch = m_treeView->AppendItem(rootBranch, "Bookmarks");
    recentBranch = m_treeView->AppendItem(rootBranch, "Recents");
    
    doUpdateActive = false;
    activeSel = nullptr;
    recentSel = nullptr;
    
    hideProps();
    m_propPanel->Hide();
    
    m_updateTimer.Start(500);
}

void BookmarkView::onUpdateTimer( wxTimerEvent& event ) {
    if (doUpdateActive) {
        doUpdateActiveList();
        
        doUpdateActive = false;
    }
}

void BookmarkView::updateTheme() {
    wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
    wxColour textColor(ThemeMgr::mgr.currentTheme->text);
    wxColour btn(ThemeMgr::mgr.currentTheme->button);
    wxColour btnHl(ThemeMgr::mgr.currentTheme->buttonHighlight);
    
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
    
    m_bookmarkButton->SetBackgroundColour(bgColor);
    m_removeButton->SetBackgroundColour(bgColor);
    m_activateButton->SetBackgroundColour(bgColor);
}


void BookmarkView::updateActiveList() {
    doUpdateActive = true;
}

void BookmarkView::doUpdateActiveList() {
    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();
    
    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
//    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    // Actives
    activeItems.erase(activeItems.begin(),activeItems.end());
    m_treeView->DeleteChildren(activeBranch);
    
    wxTreeItemId selItem = nullptr;
    for (auto demod_i : demods) {
        wxTreeItemId itm = m_treeView->AppendItem(activeBranch,demod_i->getLabel());
        activeItems[itm] = demod_i;
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
    recentItems.erase(recentItems.begin(),recentItems.end());
    m_treeView->DeleteChildren(recentBranch);
    
    for (auto bmr_i: bmRecents) {
        wxTreeItemId itm = m_treeView->AppendItem(recentBranch, bmr_i->label);
        recentItems[itm] = bmr_i;
        if (recentSel == bmr_i) {
            selItem = itm;
        }
    }

    if (selItem != nullptr) {
        m_treeView->SelectItem(selItem);
    }

    m_treeView->Enable();
    m_treeView->ExpandAll();
}

void BookmarkView::onTreeBeginLabelEdit( wxTreeEvent& event ) {
    event.Skip();
}

void BookmarkView::onTreeEndLabelEdit( wxTreeEvent& event ) {
    event.Skip();
}

void BookmarkView::onTreeActivate( wxTreeEvent& event ) {
    if (recentSel) {
        activateBookmark(recentSel);
    }
}

void BookmarkView::onTreeCollapse( wxTreeEvent& event ) {
    event.Skip();
}

void BookmarkView::onTreeExpanded( wxTreeEvent& event ) {
    event.Skip();
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
    
    m_bookmarkButton->Hide();
    m_activateButton->Hide();
    m_removeButton->Hide();
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
    
    m_bookmarkButton->Show();
    m_removeButton->Show();
    
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
    
    m_activateButton->Show();
    m_bookmarkButton->Show();
    m_removeButton->Show();
    
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
    
    m_activateButton->Show();
    m_bookmarkButton->Show();
    m_removeButton->Hide();
    
    this->Layout();
}

void BookmarkView::onTreeSelect( wxTreeEvent& event ) {
    if (activeItems.find(event.GetItem()) != activeItems.end()) {
        DemodulatorInstance *dsel = activeItems[event.GetItem()];
        m_propPanel->Show();
        activeSelection(dsel);
        wxGetApp().getDemodMgr().setActiveDemodulator(activeSel, false);
    } else if (recentItems.find(event.GetItem()) != recentItems.end()) {
        recentSel = recentItems[event.GetItem()];
        m_propPanel->Show();
        recentSelection(recentSel);
    } else {
        activeSel = nullptr;
        recentSel = nullptr;
        
        m_propPanel->Hide();
        hideProps();
        this->Layout();
        
        wxGetApp().getDemodMgr().setActiveDemodulator(activeSel, false);
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

void BookmarkView::onBookmark( wxCommandEvent& event ) {
    event.Skip();
}

void BookmarkView::onActivate( wxCommandEvent& event ) {
    if (recentSel) {
        activateBookmark(recentSel);
    }
}

void BookmarkView::onRemove( wxCommandEvent& event ) {
    if (activeSel != nullptr) {
        wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, false);
        wxGetApp().removeDemodulator(activeSel);
        wxGetApp().getDemodMgr().deleteThread(activeSel);
        activeSel = nullptr;
    }
}

