///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 23 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "DigitalConsoleFrame.h"

///////////////////////////////////////////////////////////////////////////

DigitalConsoleFrame::DigitalConsoleFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetExtraStyle( wxWS_EX_PROCESS_UI_UPDATES );
	
	wxBoxSizer* bSizer;
	bSizer = new wxBoxSizer( wxVERTICAL );
	
	m_dataView = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CHARWRAP|wxTE_MULTILINE|wxTE_NOHIDESEL|wxTE_READONLY|wxTE_WORDWRAP|wxALWAYS_SHOW_SB|wxFULL_REPAINT_ON_RESIZE|wxNO_BORDER|wxSIMPLE_BORDER|wxVSCROLL );
	m_dataView->SetExtraStyle( wxWS_EX_PROCESS_UI_UPDATES );
	m_dataView->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 76, 90, 90, false, wxEmptyString ) );
	
	bSizer->Add( m_dataView, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	m_clearButton = new wxButton( this, wxID_ANY, wxT("Clear"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_clearButton, 0, wxALL, 5 );
	
	m_copyButton = new wxButton( this, wxID_ANY, wxT("Copy"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_copyButton, 0, wxALL, 5 );
	
	m_pauseButton = new wxButton( this, wxID_ANY, wxT("Stop"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer2->Add( m_pauseButton, 0, wxEXPAND, 5 );
	
	
	bSizer->Add( bSizer2, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer );
	this->Layout();
	m_refreshTimer.SetOwner( this, wxID_ANY );
	m_refreshTimer.Start( 250 );
	
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DigitalConsoleFrame::OnClose ) );
	m_clearButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DigitalConsoleFrame::OnClear ), NULL, this );
	m_copyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DigitalConsoleFrame::OnCopy ), NULL, this );
	m_pauseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DigitalConsoleFrame::OnPause ), NULL, this );
	this->Connect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( DigitalConsoleFrame::DoRefresh ) );
}

DigitalConsoleFrame::~DigitalConsoleFrame()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DigitalConsoleFrame::OnClose ) );
	m_clearButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DigitalConsoleFrame::OnClear ), NULL, this );
	m_copyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DigitalConsoleFrame::OnCopy ), NULL, this );
	m_pauseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DigitalConsoleFrame::OnPause ), NULL, this );
	this->Disconnect( wxID_ANY, wxEVT_TIMER, wxTimerEventHandler( DigitalConsoleFrame::DoRefresh ) );
	
}
