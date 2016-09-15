#include "BookmarkView.h"
#include "CubicSDR.h"

BookmarkView::BookmarkView( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : BookmarkPanel(parent, id, pos, size, style) {
    doUpdateActive = false;
}

void BookmarkView::updateActiveList() {
    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();
    
    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    m_treeView->Disable();
    m_treeView->DeleteAllItems();
    activeItems.erase(activeItems.begin(),activeItems.end());
    
    activeBranch = m_treeView->AddRoot("Active");
    
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

void BookmarkView::onTreeSelect( wxTreeEvent& event ) {
    event.Skip();
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
    event.Skip();
}

