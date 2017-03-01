///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 23 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "BookmarkPanel.h"

///////////////////////////////////////////////////////////////////////////

BookmarkPanel::BookmarkPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_searchText = new wxTextCtrl( this, wxID_ANY, wxT("Search.."), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bSizer1->Add( m_searchText, 0, wxALL|wxEXPAND, 5 );
	
	m_clearSearchButton = new wxButton( this, wxID_ANY, wxT("Clear Search"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( m_clearSearchButton, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_treeView = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_EDIT_LABELS|wxTR_HAS_VARIABLE_ROW_HEIGHT|wxTR_HIDE_ROOT|wxTR_SINGLE );
	bSizer1->Add( m_treeView, 1, wxEXPAND, 5 );
	
	m_propPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgPropSizer;
	fgPropSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgPropSizer->AddGrowableCol( 1 );
	fgPropSizer->SetFlexibleDirection( wxBOTH );
	fgPropSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_labelLabel = new wxStaticText( m_propPanel, wxID_ANY, wxT("Label"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelLabel->Wrap( -1 );
	fgPropSizer->Add( m_labelLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_labelText = new wxTextCtrl( m_propPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	fgPropSizer->Add( m_labelText, 0, wxALL|wxEXPAND, 5 );
	
	m_frequencyLabel = new wxStaticText( m_propPanel, wxID_ANY, wxT("Freq"), wxDefaultPosition, wxDefaultSize, 0 );
	m_frequencyLabel->Wrap( -1 );
	fgPropSizer->Add( m_frequencyLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_frequencyVal = new wxStaticText( m_propPanel, wxID_ANY, wxT("FrequencyVal"), wxDefaultPosition, wxDefaultSize, 0 );
	m_frequencyVal->Wrap( -1 );
	fgPropSizer->Add( m_frequencyVal, 0, wxALL, 5 );
	
	m_bandwidthLabel = new wxStaticText( m_propPanel, wxID_ANY, wxT("BW"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bandwidthLabel->Wrap( -1 );
	fgPropSizer->Add( m_bandwidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_bandwidthVal = new wxStaticText( m_propPanel, wxID_ANY, wxT("BandwidthVal"), wxDefaultPosition, wxDefaultSize, 0 );
	m_bandwidthVal->Wrap( -1 );
	fgPropSizer->Add( m_bandwidthVal, 0, wxALL, 5 );
	
	m_modulationLabel = new wxStaticText( m_propPanel, wxID_ANY, wxT("Type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_modulationLabel->Wrap( -1 );
	fgPropSizer->Add( m_modulationLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_modulationVal = new wxStaticText( m_propPanel, wxID_ANY, wxT("TypeVal"), wxDefaultPosition, wxDefaultSize, 0 );
	m_modulationVal->Wrap( -1 );
	fgPropSizer->Add( m_modulationVal, 0, wxALL, 5 );
	
	
	m_propPanel->SetSizer( fgPropSizer );
	m_propPanel->Layout();
	fgPropSizer->Fit( m_propPanel );
	bSizer1->Add( m_propPanel, 0, wxALL|wxBOTTOM|wxEXPAND|wxTOP, 5 );
	
	m_buttonPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_buttonPanelSizer;
	m_buttonPanelSizer = new wxBoxSizer( wxVERTICAL );
	
	
	m_buttonPanel->SetSizer( m_buttonPanelSizer );
	m_buttonPanel->Layout();
	m_buttonPanelSizer->Fit( m_buttonPanel );
	bSizer1->Add( m_buttonPanel, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
	m_updateTimer.SetOwner( this, wxID_ANY );
	
	// Connect Events
	this->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( BookmarkPanel::onEnterWindow ) );
	this->Connect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( BookmarkPanel::onLeaveWindow ) );
	this->Connect( wxEVT_MOTION, wxMouseEventHandler( BookmarkPanel::onMotion ) );
	m_searchText->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( BookmarkPanel::onSearchTextFocus ), NULL, this );
	m_searchText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BookmarkPanel::onSearchText ), NULL, this );
	m_clearSearchButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BookmarkPanel::onClearSearch ), NULL, this );
    
    //VSO: Added m_treeView wxEVT_ENTER_WINDOW/wxEVT_LEAVE_WINDOW that was missing.
    m_treeView->Connect(wxEVT_ENTER_WINDOW, wxMouseEventHandler(BookmarkPanel::onEnterWindow), NULL, this);
    m_treeView->Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(BookmarkPanel::onLeaveWindow), NULL, this);
	
    m_treeView->Connect( wxEVT_MOTION, wxMouseEventHandler( BookmarkPanel::onMotion ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_BEGIN_DRAG, wxTreeEventHandler( BookmarkPanel::onTreeBeginDrag ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, wxTreeEventHandler( BookmarkPanel::onTreeBeginLabelEdit ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_END_DRAG, wxTreeEventHandler( BookmarkPanel::onTreeEndDrag ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler( BookmarkPanel::onTreeEndLabelEdit ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( BookmarkPanel::onTreeActivate ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler( BookmarkPanel::onTreeCollapse ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler( BookmarkPanel::onTreeExpanded ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_GETTOOLTIP, wxTreeEventHandler( BookmarkPanel::onTreeItemGetTooltip ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( BookmarkPanel::onTreeItemMenu ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( BookmarkPanel::onTreeSelect ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_SEL_CHANGING, wxTreeEventHandler( BookmarkPanel::onTreeSelectChanging ), NULL, this );
	m_labelText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( BookmarkPanel::onLabelText ), NULL, this );
	m_frequencyVal->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( BookmarkPanel::onDoubleClickFreq ), NULL, this );
	m_bandwidthVal->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( BookmarkPanel::onDoubleClickBandwidth ), NULL, this );
	this->Connect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( BookmarkPanel::onUpdateTimer ) );
}

BookmarkPanel::~BookmarkPanel()
{
	// Disconnect Events
	this->Disconnect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( BookmarkPanel::onEnterWindow ) );
	this->Disconnect( wxEVT_LEAVE_WINDOW, wxMouseEventHandler( BookmarkPanel::onLeaveWindow ) );
	this->Disconnect( wxEVT_MOTION, wxMouseEventHandler( BookmarkPanel::onMotion ) );
	m_searchText->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( BookmarkPanel::onSearchTextFocus ), NULL, this );
	m_searchText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BookmarkPanel::onSearchText ), NULL, this );
	m_clearSearchButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BookmarkPanel::onClearSearch ), NULL, this );
	m_treeView->Disconnect( wxEVT_MOTION, wxMouseEventHandler( BookmarkPanel::onMotion ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_BEGIN_DRAG, wxTreeEventHandler( BookmarkPanel::onTreeBeginDrag ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, wxTreeEventHandler( BookmarkPanel::onTreeBeginLabelEdit ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_END_DRAG, wxTreeEventHandler( BookmarkPanel::onTreeEndDrag ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler( BookmarkPanel::onTreeEndLabelEdit ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( BookmarkPanel::onTreeActivate ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler( BookmarkPanel::onTreeCollapse ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler( BookmarkPanel::onTreeExpanded ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_ITEM_GETTOOLTIP, wxTreeEventHandler( BookmarkPanel::onTreeItemGetTooltip ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( BookmarkPanel::onTreeItemMenu ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( BookmarkPanel::onTreeSelect ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGING, wxTreeEventHandler( BookmarkPanel::onTreeSelectChanging ), NULL, this );
	m_labelText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( BookmarkPanel::onLabelText ), NULL, this );
	m_frequencyVal->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( BookmarkPanel::onDoubleClickFreq ), NULL, this );
	m_bandwidthVal->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( BookmarkPanel::onDoubleClickBandwidth ), NULL, this );
	this->Disconnect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( BookmarkPanel::onUpdateTimer ) );
	
}
