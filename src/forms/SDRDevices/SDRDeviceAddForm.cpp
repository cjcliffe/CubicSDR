///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 27 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "SDRDeviceAddForm.h"

///////////////////////////////////////////////////////////////////////////

SDRDeviceAddForm::SDRDeviceAddForm(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer(wxVERTICAL);

	m_staticText4 = new wxStaticText(this, wxID_ANY, wxT("Manually add a SoapyRemote or SoapySDR device.  \n\nUseful for a device that is not detected automatically."), wxDefaultPosition, wxDefaultSize, 0);
	m_staticText4->Wrap(-1);
	bSizer6->Add(m_staticText4, 0, wxALL, 8);


	bSizer6->Add(0, 0, 1, wxEXPAND, 5);

	wxArrayString m_soapyModuleChoices;
	m_soapyModule = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_soapyModuleChoices, 0);
	m_soapyModule->SetSelection(0);
	bSizer6->Add(m_soapyModule, 0, wxALL, 8);


	bSizer6->Add(0, 0, 1, wxEXPAND, 5);

	m_paramLabel = new wxStaticText(this, wxID_ANY, wxT("<Parameter>"), wxDefaultPosition, wxDefaultSize, 0);
	m_paramLabel->Wrap(-1);
	bSizer6->Add(m_paramLabel, 0, wxALL, 8);

	m_paramText = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP | wxHSCROLL);
	m_paramText->SetMinSize(wxSize(-1, 48));

	bSizer6->Add(m_paramText, 1, wxALL | wxEXPAND, 8);


	bSizer6->Add(0, 0, 1, wxEXPAND, 5);

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer(wxHORIZONTAL);


	bSizer7->Add(0, 0, 1, wxEXPAND, 5);

	m_cancelButton = new wxButton(this, wxID_ANY, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer7->Add(m_cancelButton, 0, wxALL, 2);

	m_OkButton = new wxButton(this, wxID_ANY, wxT("Ok"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer7->Add(m_OkButton, 0, wxALL, 2);


	bSizer6->Add(bSizer7, 1, wxEXPAND, 8);


	bSizer6->Add(0, 0, 1, wxEXPAND, 5);


	this->SetSizer(bSizer6);
	this->Layout();

	this->Centre(wxBOTH);

	// Connect Events
	m_soapyModule->Connect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(SDRDeviceAddForm::OnSoapyModuleChanged), NULL, this);
	m_cancelButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SDRDeviceAddForm::OnCancelButton), NULL, this);
	m_OkButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SDRDeviceAddForm::OnOkButton), NULL, this);
}

SDRDeviceAddForm::~SDRDeviceAddForm()
{
	// Disconnect Events
	m_soapyModule->Disconnect(wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler(SDRDeviceAddForm::OnSoapyModuleChanged), NULL, this);
	m_cancelButton->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SDRDeviceAddForm::OnCancelButton), NULL, this);
	m_OkButton->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SDRDeviceAddForm::OnOkButton), NULL, this);

}
