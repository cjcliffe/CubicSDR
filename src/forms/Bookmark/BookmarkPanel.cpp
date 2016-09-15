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
	
	m_treeView = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE );
	bSizer1->Add( m_treeView, 5, wxEXPAND, 5 );
	
	m_propPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgPropSizer;
	fgPropSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgPropSizer->AddGrowableCol( 1 );
	fgPropSizer->SetFlexibleDirection( wxBOTH );
	fgPropSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( m_propPanel, wxID_ANY, wxT("Label"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgPropSizer->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_labelText = new wxTextCtrl( m_propPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
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
	
	m_typeVal = new wxStaticText( m_propPanel, wxID_ANY, wxT("TypeVal"), wxDefaultPosition, wxDefaultSize, 0 );
	m_typeVal->Wrap( -1 );
	fgPropSizer->Add( m_typeVal, 0, wxALL, 5 );
	
	
	m_propPanel->SetSizer( fgPropSizer );
	m_propPanel->Layout();
	fgPropSizer->Fit( m_propPanel );
	bSizer1->Add( m_propPanel, 1, wxBOTTOM|wxEXPAND|wxTOP, 5 );
	
	m_bookmarkButton = new wxButton( this, wxID_ANY, wxT("Bookmark"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( m_bookmarkButton, 0, wxALL|wxEXPAND, 5 );
	
	m_activateButton = new wxButton( this, wxID_ANY, wxT("Activate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( m_activateButton, 0, wxALL|wxEXPAND, 5 );
	
	m_removeButton = new wxButton( this, wxID_ANY, wxT("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( m_removeButton, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
	m_updateTimer.SetOwner( this, wxID_ANY );
	
	// Connect Events
	m_treeView->Connect( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, wxTreeEventHandler( BookmarkPanel::onTreeBeginLabelEdit ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler( BookmarkPanel::onTreeEndLabelEdit ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( BookmarkPanel::onTreeActivate ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler( BookmarkPanel::onTreeCollapse ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler( BookmarkPanel::onTreeExpanded ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( BookmarkPanel::onTreeSelect ), NULL, this );
	m_treeView->Connect( wxEVT_COMMAND_TREE_SEL_CHANGING, wxTreeEventHandler( BookmarkPanel::onTreeSelectChanging ), NULL, this );
	m_labelText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BookmarkPanel::onLabelText ), NULL, this );
	m_bookmarkButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BookmarkPanel::onBookmark ), NULL, this );
	m_activateButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BookmarkPanel::onActivate ), NULL, this );
	m_removeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BookmarkPanel::onRemove ), NULL, this );
	this->Connect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( BookmarkPanel::onUpdateTimer ) );
}

BookmarkPanel::~BookmarkPanel()
{
	// Disconnect Events
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, wxTreeEventHandler( BookmarkPanel::onTreeBeginLabelEdit ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_END_LABEL_EDIT, wxTreeEventHandler( BookmarkPanel::onTreeEndLabelEdit ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( BookmarkPanel::onTreeActivate ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_ITEM_COLLAPSED, wxTreeEventHandler( BookmarkPanel::onTreeCollapse ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_ITEM_EXPANDED, wxTreeEventHandler( BookmarkPanel::onTreeExpanded ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( BookmarkPanel::onTreeSelect ), NULL, this );
	m_treeView->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGING, wxTreeEventHandler( BookmarkPanel::onTreeSelectChanging ), NULL, this );
	m_labelText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( BookmarkPanel::onLabelText ), NULL, this );
	m_bookmarkButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BookmarkPanel::onBookmark ), NULL, this );
	m_activateButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BookmarkPanel::onActivate ), NULL, this );
	m_removeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( BookmarkPanel::onRemove ), NULL, this );
	this->Disconnect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( BookmarkPanel::onUpdateTimer ) );
	
}
