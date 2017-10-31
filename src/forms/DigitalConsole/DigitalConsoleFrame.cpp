///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 27 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "DigitalConsoleFrame.h"

///////////////////////////////////////////////////////////////////////////

DigitalConsoleFrame::DigitalConsoleFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);
	this->SetExtraStyle(wxWS_EX_PROCESS_UI_UPDATES);

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* dataViewSizer;
	dataViewSizer = new wxBoxSizer(wxVERTICAL);

	m_dataView = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_CHARWRAP | wxTE_MULTILINE | wxTE_NOHIDESEL | wxTE_READONLY | wxTE_WORDWRAP | wxALWAYS_SHOW_SB | wxFULL_REPAINT_ON_RESIZE | wxNO_BORDER | wxSIMPLE_BORDER | wxVSCROLL);
	m_dataView->SetExtraStyle(wxWS_EX_PROCESS_UI_UPDATES);
	m_dataView->SetFont(wxFont(wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString));

	dataViewSizer->Add(m_dataView, 1, wxEXPAND, 5);


	mainSizer->Add(dataViewSizer, 1, wxEXPAND, 5);

	wxBoxSizer* buttonSizer;
	buttonSizer = new wxBoxSizer(wxHORIZONTAL);

	m_clearButton = new wxButton(this, wxID_ANY, wxT("Clear"), wxDefaultPosition, wxDefaultSize, 0);
	buttonSizer->Add(m_clearButton, 1, wxEXPAND, 5);

	m_copyButton = new wxButton(this, wxID_ANY, wxT("Copy"), wxDefaultPosition, wxDefaultSize, 0);
	buttonSizer->Add(m_copyButton, 1, wxEXPAND, 5);

	m_pauseButton = new wxButton(this, wxID_ANY, wxT("Stop"), wxDefaultPosition, wxDefaultSize, 0);
	buttonSizer->Add(m_pauseButton, 1, wxEXPAND, 5);


	mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 5);


	this->SetSizer(mainSizer);
	this->Layout();
	m_refreshTimer.SetOwner(this, wxID_ANY);
	m_refreshTimer.Start(250);


	this->Centre(wxBOTH);

	// Connect Events
	this->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(DigitalConsoleFrame::OnClose));
	m_clearButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DigitalConsoleFrame::OnClear), NULL, this);
	m_copyButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DigitalConsoleFrame::OnCopy), NULL, this);
	m_pauseButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DigitalConsoleFrame::OnPause), NULL, this);
	this->Connect(wxID_ANY, wxEVT_TIMER, wxTimerEventHandler(DigitalConsoleFrame::DoRefresh));
}

DigitalConsoleFrame::~DigitalConsoleFrame()
{
	// Disconnect Events
	this->Disconnect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(DigitalConsoleFrame::OnClose));
	m_clearButton->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DigitalConsoleFrame::OnClear), NULL, this);
	m_copyButton->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DigitalConsoleFrame::OnCopy), NULL, this);
	m_pauseButton->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(DigitalConsoleFrame::OnPause), NULL, this);
	this->Disconnect(wxID_ANY, wxEVT_TIMER, wxTimerEventHandler(DigitalConsoleFrame::DoRefresh));

}
