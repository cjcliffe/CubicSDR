///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 23 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "SDRDevicesForm.h"

///////////////////////////////////////////////////////////////////////////

devFrame::devFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	devStatusBar = this->CreateStatusBar( 1, wxST_SIZEGRIP, wxID_ANY );
	wxBoxSizer* devFrameSizer;
	devFrameSizer = new wxBoxSizer( wxVERTICAL );
	
	m_panel3 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	m_panel6 = new wxPanel( m_panel3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	devTree = new wxTreeCtrl( m_panel6, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE );
	devTree->Enable( false );
	
	bSizer6->Add( devTree, 1, wxEXPAND|wxALIGN_RIGHT, 5 );
	
	m_panel4 = new wxPanel( m_panel6, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	m_refreshButton = new wxButton( m_panel4, wxID_ANY, wxT("Refresh"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_refreshButton, 0, wxALL, 5 );
	
	m_addRemoteButton = new wxButton( m_panel4, wxID_ANY, wxT("Add Remote"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_addRemoteButton, 1, wxALL, 5 );
	
	m_useSelectedButton = new wxButton( m_panel4, wxID_ANY, wxT("Use Selected"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_useSelectedButton, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_panel4->SetSizer( bSizer5 );
	m_panel4->Layout();
	bSizer5->Fit( m_panel4 );
	bSizer6->Add( m_panel4, 0, wxEXPAND, 5 );
	
	
	m_panel6->SetSizer( bSizer6 );
	m_panel6->Layout();
	bSizer6->Fit( m_panel6 );
	bSizer4->Add( m_panel6, 1, wxEXPAND | wxALL, 5 );
	
	m_panel61 = new wxPanel( m_panel3, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( m_panel61, wxID_ANY, wxT("SoapySDR Device Options"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer7->Add( m_staticText1, 0, wxALL, 5 );
	
	m_propertyGrid = new wxPropertyGrid(m_panel61, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE);
	bSizer7->Add( m_propertyGrid, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panel61->SetSizer( bSizer7 );
	m_panel61->Layout();
	bSizer7->Fit( m_panel61 );
	bSizer4->Add( m_panel61, 1, wxEXPAND | wxALL, 5 );
	
	
	m_panel3->SetSizer( bSizer4 );
	m_panel3->Layout();
	bSizer4->Fit( m_panel3 );
	devFrameSizer->Add( m_panel3, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( devFrameSizer );
	this->Layout();
	m_deviceTimer.SetOwner( this, wxID_ANY );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( devFrame::OnClose ) );
	devTree->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( devFrame::OnTreeDoubleClick ), NULL, this );
	devTree->Connect( wxEVT_COMMAND_TREE_DELETE_ITEM, wxTreeEventHandler( devFrame::OnDeleteItem ), NULL, this );
	devTree->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( devFrame::OnSelectionChanged ), NULL, this );
	m_refreshButton->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( devFrame::OnRefreshDevices ), NULL, this );
	m_addRemoteButton->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( devFrame::OnAddRemote ), NULL, this );
	m_useSelectedButton->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( devFrame::OnUseSelected ), NULL, this );
	this->Connect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( devFrame::OnDeviceTimer ) );
}

devFrame::~devFrame()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( devFrame::OnClose ) );
	devTree->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( devFrame::OnTreeDoubleClick ), NULL, this );
	devTree->Disconnect( wxEVT_COMMAND_TREE_DELETE_ITEM, wxTreeEventHandler( devFrame::OnDeleteItem ), NULL, this );
	devTree->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( devFrame::OnSelectionChanged ), NULL, this );
	m_refreshButton->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( devFrame::OnRefreshDevices ), NULL, this );
	m_addRemoteButton->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( devFrame::OnAddRemote ), NULL, this );
	m_useSelectedButton->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( devFrame::OnUseSelected ), NULL, this );
	this->Disconnect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( devFrame::OnDeviceTimer ) );
	
}
