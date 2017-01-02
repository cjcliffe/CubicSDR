///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 23 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "ActionDialogBase.h"

///////////////////////////////////////////////////////////////////////////

ActionDialogBase::ActionDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_questionText = new wxStaticText( this, wxID_ANY, wxT("Question"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_questionText->Wrap( -1 );
	mainSizer->Add( m_questionText, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* buttonSizer;
	buttonSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_cancelButton = new wxButton( this, wxID_ANY, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonSizer->Add( m_cancelButton, 1, wxALL|wxEXPAND, 5 );
	
	m_okButton = new wxButton( this, wxID_ANY, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	buttonSizer->Add( m_okButton, 1, wxALL|wxEXPAND, 5 );
	
	
	mainSizer->Add( buttonSizer, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_cancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ActionDialogBase::onClickCancel ), NULL, this );
	m_okButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ActionDialogBase::onClickOK ), NULL, this );
}

ActionDialogBase::~ActionDialogBase()
{
	// Disconnect Events
	m_cancelButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ActionDialogBase::onClickCancel ), NULL, this );
	m_okButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ActionDialogBase::onClickOK ), NULL, this );
	
}
