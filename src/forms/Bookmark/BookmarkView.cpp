#include "BookmarkView.h"
#include "CubicSDR.h"

BookmarkView::BookmarkView( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : BookmarkPanel(parent, id, pos, size, style) {

    activeBranch = m_treeView->AddRoot("Active");
    doUpdateActive = false;
    activeSel = nullptr;
    m_updateTimer.Start(500);
    hideProps();
    m_propPanel->Hide();
}

void BookmarkView::onUpdateTimer( wxTimerEvent& event ) {
    if (doUpdateActive) {
        doUpdateActiveList();
        
        doUpdateActive = false;
    }
}

void BookmarkView::updateActiveList() {
    doUpdateActive = true;
}

void BookmarkView::doUpdateActiveList() {
    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();
    
//    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
//    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    activeItems.erase(activeItems.begin(),activeItems.end());
    m_treeView->DeleteChildren(activeBranch);
    
    for (auto demod_i : demods) {
        wxTreeItemId itm = m_treeView->AppendItem(activeBranch,demod_i->getLabel());
        activeItems[itm] = demod_i;
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
    event.Skip();
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

void BookmarkView::onTreeSelect( wxTreeEvent& event ) {
    if (activeItems.find(event.GetItem()) != activeItems.end()) {
        DemodulatorInstance *dsel = activeItems[event.GetItem()];
        m_propPanel->Show();
        activeSelection(dsel);
        wxGetApp().getDemodMgr().setActiveDemodulator(activeSel, false);
    } else {
        activeSel = nullptr;
        
        m_propPanel->Hide();
        hideProps();
        this->Layout();
        
        wxGetApp().getDemodMgr().setActiveDemodulator(activeSel, false);
        event.Skip();
    }
}

void BookmarkView::onTreeSelectChanging( wxTreeEvent& event ) {
    event.Skip();
}

void BookmarkView::onLabelText( wxCommandEvent& event ) {
    event.Skip();
}

void BookmarkView::onBookmark( wxCommandEvent& event ) {
    event.Skip();
}

void BookmarkView::onActivate( wxCommandEvent& event ) {
    event.Skip();
}

void BookmarkView::onRemove( wxCommandEvent& event ) {
    if (activeSel != nullptr) {
        wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, false);
        wxGetApp().removeDemodulator(activeSel);
        wxGetApp().getDemodMgr().deleteThread(activeSel);
        activeSel = nullptr;
    } else {
        event.Skip();
    }
}

