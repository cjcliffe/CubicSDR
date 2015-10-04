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
	devFrameSizer = new wxBoxSizer( wxHORIZONTAL );
	
	devTree = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE );
	devFrameSizer->Add( devTree, 1, wxEXPAND, 5 );
	
	devTabs = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	devInfoPanel = new wxPanel( devTabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* devInfoSizer;
	devInfoSizer = new wxBoxSizer( wxVERTICAL );
	
	m_DevInfoList = new wxListCtrl( devInfoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ICON );
	devInfoSizer->Add( m_DevInfoList, 1, wxEXPAND, 5 );
	
	
	devInfoPanel->SetSizer( devInfoSizer );
	devInfoPanel->Layout();
	devInfoSizer->Fit( devInfoPanel );
	devTabs->AddPage( devInfoPanel, wxT("Device"), false );
	devParamsPanel = new wxPanel( devTabs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* devParamsSizer;
	devParamsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_ParamInfoList = new wxListCtrl( devParamsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ICON );
	devParamsSizer->Add( m_ParamInfoList, 1, wxEXPAND, 5 );
	
	
	devParamsPanel->SetSizer( devParamsSizer );
	devParamsPanel->Layout();
	devParamsSizer->Fit( devParamsPanel );
	devTabs->AddPage( devParamsPanel, wxT("Parameters"), false );
	
	devFrameSizer->Add( devTabs, 2, wxEXPAND, 5 );
	
	
	this->SetSizer( devFrameSizer );
	this->Layout();
	devMenuBar = new wxMenuBar( 0 );
	devFileMenu = new wxMenu();
	devMenuBar->Append( devFileMenu, wxT("File") ); 
	
	devEditMenu = new wxMenu();
	devMenuBar->Append( devEditMenu, wxT("Edit") ); 
	
	this->SetMenuBar( devMenuBar );
	
	
	this->Centre( wxBOTH );
}

devFrame::~devFrame()
{
}
