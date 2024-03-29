///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "AboutDialogBase.h"

///////////////////////////////////////////////////////////////////////////

AboutDialogBase::AboutDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* dlgSizer;
	dlgSizer = new wxBoxSizer( wxVERTICAL );

	m_hPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_hSizer;
	m_hSizer = new wxBoxSizer( wxHORIZONTAL );

	m_hSizer->SetMinSize( wxSize( -1,42 ) );
	m_appName = new wxStaticText( m_hPanel, wxID_ANY, wxT("CubicSDR"), wxDefaultPosition, wxDefaultSize, 0 );
	m_appName->Wrap( -1 );
	m_appName->SetFont( wxFont( 20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_hSizer->Add( m_appName, 1, wxALL|wxEXPAND, 6 );


	m_hPanel->SetSizer( m_hSizer );
	m_hPanel->Layout();
	m_hSizer->Fit( m_hPanel );
	dlgSizer->Add( m_hPanel, 0, wxALL|wxEXPAND, 5 );

	m_aboutNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_dbPanel = new wxPanel( m_aboutNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_dbScrollSizer;
	m_dbScrollSizer = new wxBoxSizer( wxVERTICAL );

	m_dbScroll = new wxScrolledWindow( m_dbPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_dbScroll->SetScrollRate( 5, 5 );
	wxBoxSizer* m_dbPane;
	m_dbPane = new wxBoxSizer( wxVERTICAL );

	m_dbPanel1 = new wxPanel( m_dbScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* m_dbSizer;
	m_dbSizer = new wxFlexGridSizer( 0, 3, 2, 20 );
	m_dbSizer->SetFlexibleDirection( wxBOTH );
	m_dbSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_dbHeader = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("Developed By"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_dbHeader->Wrap( -1 );
	m_dbHeader->SetFont( wxFont( 15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_dbSizer->Add( m_dbHeader, 0, wxALL, 5 );

	m_dbGHHeader = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("GitHub"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbGHHeader->Wrap( -1 );
	m_dbGHHeader->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_dbSizer->Add( m_dbGHHeader, 0, wxALL, 5 );

	m_dbTwitter = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("Twitter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbTwitter->Wrap( -1 );
	m_dbTwitter->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_dbSizer->Add( m_dbTwitter, 0, wxALL, 5 );

	m_dbCharlesCliffe = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("Charles J. Cliffe"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbCharlesCliffe->Wrap( -1 );
	m_dbSizer->Add( m_dbCharlesCliffe, 0, wxALL, 5 );

	m_dbghCC = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("@cjcliffe"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbghCC->Wrap( -1 );
	m_dbSizer->Add( m_dbghCC, 0, wxALL, 5 );

	m_dbtCC = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("@ccliffe"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbtCC->Wrap( -1 );
	m_dbSizer->Add( m_dbtCC, 0, wxALL, 5 );

	m_dbVincentSonnier = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("Vincent Sonnier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbVincentSonnier->Wrap( -1 );
	m_dbSizer->Add( m_dbVincentSonnier, 0, wxALL, 5 );

	m_dbghVS = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("@vsonnier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbghVS->Wrap( -1 );
	m_dbSizer->Add( m_dbghVS, 0, wxALL, 5 );

	m_dbtVS = new wxStaticText( m_dbPanel1, wxID_ANY, wxT("@VincentSonnier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbtVS->Wrap( -1 );
	m_dbSizer->Add( m_dbtVS, 0, wxALL, 5 );


	m_dbPanel1->SetSizer( m_dbSizer );
	m_dbPanel1->Layout();
	m_dbSizer->Fit( m_dbPanel1 );
	m_dbPane->Add( m_dbPanel1, 0, wxALL|wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );

	m_dbDivider1 = new wxStaticLine( m_dbScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_dbPane->Add( m_dbDivider1, 0, wxALL|wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 10 );

	m_dbPanel2 = new wxPanel( m_dbScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* m_cSizer;
	m_cSizer = new wxFlexGridSizer( 0, 2, 2, 20 );
	m_cSizer->SetFlexibleDirection( wxBOTH );
	m_cSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

	m_cContributorsHeader = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Contributors"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_cContributorsHeader->Wrap( -1 );
	m_cContributorsHeader->SetFont( wxFont( 15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_cSizer->Add( m_cContributorsHeader, 0, wxALL, 5 );

	m_cGitHub = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("GitHub"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cGitHub->Wrap( -1 );
	m_cGitHub->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_cSizer->Add( m_cGitHub, 0, wxALL, 5 );

	m_cCorneLukken = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Corne Lukken"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cCorneLukken->Wrap( -1 );
	m_cSizer->Add( m_cCorneLukken, 0, wxALL, 5 );

	m_cghCL = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@Dantali0n"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghCL->Wrap( -1 );
	m_cSizer->Add( m_cghCL, 0, wxALL, 5 );

	m_cStainislawPitucha = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Stanisław Pitucha"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cStainislawPitucha->Wrap( -1 );
	m_cSizer->Add( m_cStainislawPitucha, 0, wxALL, 5 );

	m_cghSP = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@viraptor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghSP->Wrap( -1 );
	m_cSizer->Add( m_cghSP, 0, wxALL, 5 );

	m_cTomSwartz = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Tom Swartz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cTomSwartz->Wrap( -1 );
	m_cSizer->Add( m_cTomSwartz, 0, wxALL, 5 );

	m_cghTS = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@tomswartz07"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghTS->Wrap( -1 );
	m_cSizer->Add( m_cghTS, 0, wxALL, 5 );

	m_cStefanTalpalaru = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Ștefan Talpalaru"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cStefanTalpalaru->Wrap( -1 );
	m_cSizer->Add( m_cStefanTalpalaru, 0, wxALL, 5 );

	m_cghST = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@stefantalpalaru"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghST->Wrap( -1 );
	m_cSizer->Add( m_cghST, 0, wxALL, 5 );

	m_cDellRaySackett = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Dell-Ray Sackett"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cDellRaySackett->Wrap( -1 );
	m_cSizer->Add( m_cDellRaySackett, 0, wxALL, 5 );

	m_cghDRS = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@lospheris"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghDRS->Wrap( -1 );
	m_cSizer->Add( m_cghDRS, 0, wxALL, 5 );

	m_cJiangWei = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Jiang Wei"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cJiangWei->Wrap( -1 );
	m_cSizer->Add( m_cJiangWei, 0, wxALL, 5 );

	m_cghJW = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@jocover"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghJW->Wrap( -1 );
	m_cSizer->Add( m_cghJW, 0, wxALL, 5 );

	m_cInfinityCyberworks = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Infinity Cyberworks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cInfinityCyberworks->Wrap( -1 );
	m_cSizer->Add( m_cInfinityCyberworks, 0, wxALL, 5 );

	m_cghIC = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@infinitycyberworks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghIC->Wrap( -1 );
	m_cSizer->Add( m_cghIC, 0, wxALL, 5 );

	m_cCrisMotch = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Chris Motch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cCrisMotch->Wrap( -1 );
	m_cSizer->Add( m_cCrisMotch, 0, wxALL, 5 );

	m_cghCM = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@bodrick"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghCM->Wrap( -1 );
	m_cSizer->Add( m_cghCM, 0, wxALL, 5 );

	m_cAntiHax = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Antihax"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cAntiHax->Wrap( -1 );
	m_cSizer->Add( m_cAntiHax, 0, wxALL, 5 );

	m_cghAH = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@antihax"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghAH->Wrap( -1 );
	m_cSizer->Add( m_cghAH, 0, wxALL, 5 );

	m_cRainbow = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Rainbow"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cRainbow->Wrap( -1 );
	m_cSizer->Add( m_cRainbow, 0, wxALL, 5 );

	m_cghRBW = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@ra1nb0w"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghRBW->Wrap( -1 );
	m_cSizer->Add( m_cghRBW, 0, wxALL, 5 );

	m_cMariuszRyndzionek = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Mariusz Ryndzionek"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cMariuszRyndzionek->Wrap( -1 );
	m_cSizer->Add( m_cMariuszRyndzionek, 0, wxALL, 5 );

	m_cghMR = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@mryndzionek"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghMR->Wrap( -1 );
	m_cSizer->Add( m_cghMR, 0, wxALL, 5 );

	m_cDrahosj = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("drahosj"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cDrahosj->Wrap( -1 );
	m_cSizer->Add( m_cDrahosj, 0, wxALL, 5 );

	m_cghDra = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@drahosj"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghDra->Wrap( -1 );
	m_cSizer->Add( m_cghDra, 0, wxALL, 5 );

	m_cBenoitAllard = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Benoît Allard"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cBenoitAllard->Wrap( -1 );
	m_cSizer->Add( m_cBenoitAllard, 0, wxALL, 5 );

	m_cghBA = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@benallard"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghBA->Wrap( -1 );
	m_cSizer->Add( m_cghBA, 0, wxALL, 5 );

	m_cDianeBruce = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Diane Bruce"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cDianeBruce->Wrap( -1 );
	m_cSizer->Add( m_cDianeBruce, 0, wxALL, 5 );

	m_cghDiB = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@DianeBruce"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghDiB->Wrap( -1 );
	m_cSizer->Add( m_cghDiB, 0, wxALL, 5 );

	m_cPaulColby = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Paul Colby"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cPaulColby->Wrap( -1 );
	m_cSizer->Add( m_cPaulColby, 0, wxALL, 5 );

	m_cghCACRI = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@colbyAtCRI"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghCACRI->Wrap( -1 );
	m_cSizer->Add( m_cghCACRI, 0, wxALL, 5 );

	m_cclassabbyamp = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("classabbyamp"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cclassabbyamp->Wrap( -1 );
	m_cSizer->Add( m_cclassabbyamp, 0, wxALL, 5 );

	m_cghclassabbyamp = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@classabbyamp"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghclassabbyamp->Wrap( -1 );
	m_cSizer->Add( m_cghclassabbyamp, 0, wxALL, 5 );

	m_cSDRplay = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("SDRplay"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cSDRplay->Wrap( -1 );
	m_cSizer->Add( m_cSDRplay, 0, wxALL, 5 );

	m_cghSDRplay = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@SDRplay"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghSDRplay->Wrap( -1 );
	m_cSizer->Add( m_cghSDRplay, 0, wxALL, 5 );

	m_cdforsi = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("dforsi"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cdforsi->Wrap( -1 );
	m_cSizer->Add( m_cdforsi, 0, wxALL, 5 );

	m_cghdforsi = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@dforsi"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghdforsi->Wrap( -1 );
	m_cSizer->Add( m_cghdforsi, 0, wxALL, 5 );

	m_cMagalex2x14 = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("Aleksey Makarenko"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cMagalex2x14->Wrap( -1 );
	m_cSizer->Add( m_cMagalex2x14, 0, wxALL, 5 );

	m_cghMagalex2x14 = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@Magalex2x14"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghMagalex2x14->Wrap( -1 );
	m_cSizer->Add( m_cghMagalex2x14, 0, wxALL, 5 );

	m_cjawatson = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("James Watson"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cjawatson->Wrap( -1 );
	m_cSizer->Add( m_cjawatson, 0, wxALL, 5 );

	m_cghjawatson = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@jawatson"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghjawatson->Wrap( -1 );
	m_cSizer->Add( m_cghjawatson, 0, wxALL, 5 );

	m_cghf4grx = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("f4grx"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghf4grx->Wrap( -1 );
	m_cSizer->Add( m_cghf4grx, 0, wxALL, 5 );

	m_cf4grx = new wxStaticText( m_dbPanel2, wxID_ANY, wxT("@f4grx"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cf4grx->Wrap( -1 );
	m_cSizer->Add( m_cf4grx, 0, wxALL, 5 );


	m_dbPanel2->SetSizer( m_cSizer );
	m_dbPanel2->Layout();
	m_cSizer->Fit( m_dbPanel2 );
	m_dbPane->Add( m_dbPanel2, 1, wxALL|wxEXPAND|wxRESERVE_SPACE_EVEN_IF_HIDDEN, 5 );


	m_dbScroll->SetSizer( m_dbPane );
	m_dbScroll->Layout();
	m_dbPane->Fit( m_dbScroll );
	m_dbScrollSizer->Add( m_dbScroll, 1, wxEXPAND | wxALL, 5 );


	m_dbPanel->SetSizer( m_dbScrollSizer );
	m_dbPanel->Layout();
	m_dbScrollSizer->Fit( m_dbPanel );
	m_aboutNotebook->AddPage( m_dbPanel, wxT("Developers"), true );
	m_dPanel = new wxPanel( m_aboutNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_dScrollSizer;
	m_dScrollSizer = new wxBoxSizer( wxVERTICAL );

	m_dScroll = new wxScrolledWindow( m_dPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_dScroll->SetScrollRate( 5, 5 );
	wxBoxSizer* m_dSizer;
	m_dSizer = new wxBoxSizer( wxVERTICAL );

	m_dHeader = new wxStaticText( m_dScroll, wxID_ANY, wxT("Thanks to everyone who donated at cubicsdr.com!"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_dHeader->Wrap( -1 );
	m_dHeader->SetFont( wxFont( 14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_dSizer->Add( m_dHeader, 0, wxALL, 5 );

	m_dDivider1 = new wxStaticLine( m_dScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_dSizer->Add( m_dDivider1, 0, wxEXPAND | wxALL, 5 );

	m_dSDRplay = new wxStaticText( m_dScroll, wxID_ANY, wxT("SDRplay / sdrplay.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dSDRplay->Wrap( -1 );
	m_dSizer->Add( m_dSDRplay, 0, wxALL, 5 );

	m_dMichaelLadd = new wxStaticText( m_dScroll, wxID_ANY, wxT("Michael Ladd"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMichaelLadd->Wrap( -1 );
	m_dSizer->Add( m_dMichaelLadd, 0, wxALL, 5 );

	m_dAutoMotiveTemplates = new wxStaticText( m_dScroll, wxID_ANY, wxT("Automotive Templates"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dAutoMotiveTemplates->Wrap( -1 );
	m_dSizer->Add( m_dAutoMotiveTemplates, 0, wxALL, 5 );

	m_dJorgeMorales = new wxStaticText( m_dScroll, wxID_ANY, wxT("Jorge Morales"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJorgeMorales->Wrap( -1 );
	m_dSizer->Add( m_dJorgeMorales, 0, wxALL, 5 );

	m_dMichaelRooke = new wxStaticText( m_dScroll, wxID_ANY, wxT("Michael Rooke"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMichaelRooke->Wrap( -1 );
	m_dSizer->Add( m_dMichaelRooke, 0, wxALL, 5 );

	m_dErikWied = new wxStaticText( m_dScroll, wxID_ANY, wxT("Erik Mikkel Wied"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dErikWied->Wrap( -1 );
	m_dSizer->Add( m_dErikWied, 0, wxALL, 5 );

	m_dChristopherEsser = new wxStaticText( m_dScroll, wxID_ANY, wxT("Christopher Esser"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dChristopherEsser->Wrap( -1 );
	m_dSizer->Add( m_dChristopherEsser, 0, wxALL, 5 );

	m_dTNCOM = new wxStaticText( m_dScroll, wxID_ANY, wxT("TNCOM"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dTNCOM->Wrap( -1 );
	m_dSizer->Add( m_dTNCOM, 0, wxALL, 5 );

	m_dAlexanderSadleir = new wxStaticText( m_dScroll, wxID_ANY, wxT("Alexander Sadleir"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dAlexanderSadleir->Wrap( -1 );
	m_dSizer->Add( m_dAlexanderSadleir, 0, wxALL, 5 );

	m_dRobertDuering = new wxStaticText( m_dScroll, wxID_ANY, wxT("Robert Duering"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRobertDuering->Wrap( -1 );
	m_dSizer->Add( m_dRobertDuering, 0, wxALL, 5 );

	m_dJimDeitch = new wxStaticText( m_dScroll, wxID_ANY, wxT("Jim Deitch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJimDeitch->Wrap( -1 );
	m_dSizer->Add( m_dJimDeitch, 0, wxALL, 5 );

	m_dNooElec = new wxStaticText( m_dScroll, wxID_ANY, wxT("NooElec Inc. / nooelec.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dNooElec->Wrap( -1 );
	m_dSizer->Add( m_dNooElec, 0, wxALL, 5 );

	m_dDavidAhlgren = new wxStaticText( m_dScroll, wxID_ANY, wxT("David Ahlgren"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDavidAhlgren->Wrap( -1 );
	m_dSizer->Add( m_dDavidAhlgren, 0, wxALL, 5 );

	m_dRonaldCook = new wxStaticText( m_dScroll, wxID_ANY, wxT("Ronald Cook"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRonaldCook->Wrap( -1 );
	m_dSizer->Add( m_dRonaldCook, 0, wxALL, 5 );

	m_dEricPeterson = new wxStaticText( m_dScroll, wxID_ANY, wxT("Eric Peterson"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dEricPeterson->Wrap( -1 );
	m_dSizer->Add( m_dEricPeterson, 0, wxALL, 5 );

	m_dGeoDistributing = new wxStaticText( m_dScroll, wxID_ANY, wxT("Geo Distributing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dGeoDistributing->Wrap( -1 );
	m_dSizer->Add( m_dGeoDistributing, 0, wxALL, 5 );

	m_dJamesCarson = new wxStaticText( m_dScroll, wxID_ANY, wxT("James Carson"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJamesCarson->Wrap( -1 );
	m_dSizer->Add( m_dJamesCarson, 0, wxALL, 5 );

	m_dCraigWilliams = new wxStaticText( m_dScroll, wxID_ANY, wxT("Craig Williams"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dCraigWilliams->Wrap( -1 );
	m_dSizer->Add( m_dCraigWilliams, 0, wxALL, 5 );

	m_dRudolfShaffer = new wxStaticText( m_dScroll, wxID_ANY, wxT("Rudolf Schaffer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRudolfShaffer->Wrap( -1 );
	m_dSizer->Add( m_dRudolfShaffer, 0, wxALL, 5 );

	m_dJohnKaton = new wxStaticText( m_dScroll, wxID_ANY, wxT("John Katon"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJohnKaton->Wrap( -1 );
	m_dSizer->Add( m_dJohnKaton, 0, wxALL, 5 );

	m_dVincentSonnier = new wxStaticText( m_dScroll, wxID_ANY, wxT("Vincent Sonnier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dVincentSonnier->Wrap( -1 );
	m_dSizer->Add( m_dVincentSonnier, 0, wxALL, 5 );

	m_dCorq = new wxStaticText( m_dScroll, wxID_ANY, wxT("corq's auctions/L. Easterly LTD (x 5)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dCorq->Wrap( -1 );
	m_dSizer->Add( m_dCorq, 0, wxALL, 5 );

	m_dIvanAlekseev = new wxStaticText( m_dScroll, wxID_ANY, wxT("Ivan Alekseev"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dIvanAlekseev->Wrap( -1 );
	m_dSizer->Add( m_dIvanAlekseev, 0, wxALL, 5 );

	m_dOleJorgenKolsrud = new wxStaticText( m_dScroll, wxID_ANY, wxT("Ole-Jørgen Næss Kolsrud"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dOleJorgenKolsrud->Wrap( -1 );
	m_dSizer->Add( m_dOleJorgenKolsrud, 0, wxALL, 5 );

	m_dHenrikJagemyr = new wxStaticText( m_dScroll, wxID_ANY, wxT("Henrik Jagemyr"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dHenrikJagemyr->Wrap( -1 );
	m_dSizer->Add( m_dHenrikJagemyr, 0, wxALL, 5 );

	m_dPeterHaines = new wxStaticText( m_dScroll, wxID_ANY, wxT("Peter Haines"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dPeterHaines->Wrap( -1 );
	m_dSizer->Add( m_dPeterHaines, 0, wxALL, 5 );

	m_dLeonAbrassart = new wxStaticText( m_dScroll, wxID_ANY, wxT("Leon Abrassart"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dLeonAbrassart->Wrap( -1 );
	m_dSizer->Add( m_dLeonAbrassart, 0, wxALL, 5 );

	m_dGeorgeTalbot = new wxStaticText( m_dScroll, wxID_ANY, wxT("George Alan Talbot"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dGeorgeTalbot->Wrap( -1 );
	m_dSizer->Add( m_dGeorgeTalbot, 0, wxALL, 5 );

	m_dFranciscoPuerta = new wxStaticText( m_dScroll, wxID_ANY, wxT("Francisco Borja Marcos de la Puerta"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dFranciscoPuerta->Wrap( -1 );
	m_dSizer->Add( m_dFranciscoPuerta, 0, wxALL, 5 );

	m_dRonaldLundeen = new wxStaticText( m_dScroll, wxID_ANY, wxT("Ronald A. Lundeen"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRonaldLundeen->Wrap( -1 );
	m_dSizer->Add( m_dRonaldLundeen, 0, wxALL, 5 );

	m_dWalterHorbert = new wxStaticText( m_dScroll, wxID_ANY, wxT("Walter Horbert"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dWalterHorbert->Wrap( -1 );
	m_dSizer->Add( m_dWalterHorbert, 0, wxALL, 5 );

	m_dWilliamLD = new wxStaticText( m_dScroll, wxID_ANY, wxT("William Lloyd-Davies"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dWilliamLD->Wrap( -1 );
	m_dSizer->Add( m_dWilliamLD, 0, wxALL, 5 );

	m_dBratislavArandjelovic = new wxStaticText( m_dScroll, wxID_ANY, wxT("Bratislav Arandjelovic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dBratislavArandjelovic->Wrap( -1 );
	m_dSizer->Add( m_dBratislavArandjelovic, 0, wxALL, 5 );

	m_dGaryMartin = new wxStaticText( m_dScroll, wxID_ANY, wxT("Gary Martin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dGaryMartin->Wrap( -1 );
	m_dSizer->Add( m_dGaryMartin, 0, wxALL, 5 );

	m_dEinarsRepse = new wxStaticText( m_dScroll, wxID_ANY, wxT("Einars Repse"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dEinarsRepse->Wrap( -1 );
	m_dSizer->Add( m_dEinarsRepse, 0, wxALL, 5 );

	m_dTimothyGatton = new wxStaticText( m_dScroll, wxID_ANY, wxT("Timothy Gatton"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dTimothyGatton->Wrap( -1 );
	m_dSizer->Add( m_dTimothyGatton, 0, wxALL, 5 );

	m_dStephenCuccio = new wxStaticText( m_dScroll, wxID_ANY, wxT("Stephen Cuccio"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dStephenCuccio->Wrap( -1 );
	m_dSizer->Add( m_dStephenCuccio, 0, wxALL, 5 );

	m_dKeshavlalPatel = new wxStaticText( m_dScroll, wxID_ANY, wxT("Keshavlal Patel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dKeshavlalPatel->Wrap( -1 );
	m_dSizer->Add( m_dKeshavlalPatel, 0, wxALL, 5 );

	m_dBobSchatzman = new wxStaticText( m_dScroll, wxID_ANY, wxT("Bob Schatzman"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dBobSchatzman->Wrap( -1 );
	m_dSizer->Add( m_dBobSchatzman, 0, wxALL, 5 );

	m_dRobertRoss = new wxStaticText( m_dScroll, wxID_ANY, wxT("Robert Ross"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRobertRoss->Wrap( -1 );
	m_dSizer->Add( m_dRobertRoss, 0, wxALL, 5 );

	m_dRobertoBellotti = new wxStaticText( m_dScroll, wxID_ANY, wxT("Roberto Bellotti"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRobertoBellotti->Wrap( -1 );
	m_dSizer->Add( m_dRobertoBellotti, 0, wxALL, 5 );

	m_dSergeVanderTorre = new wxStaticText( m_dScroll, wxID_ANY, wxT("Serge Van der Torre"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dSergeVanderTorre->Wrap( -1 );
	m_dSizer->Add( m_dSergeVanderTorre, 0, wxALL, 5 );

	m_dDieterSchneider = new wxStaticText( m_dScroll, wxID_ANY, wxT("Dieter Schneider"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDieterSchneider->Wrap( -1 );
	m_dSizer->Add( m_dDieterSchneider, 0, wxALL, 5 );

	m_dPetrikaJaneku = new wxStaticText( m_dScroll, wxID_ANY, wxT("Petrika Janeku"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dPetrikaJaneku->Wrap( -1 );
	m_dSizer->Add( m_dPetrikaJaneku, 0, wxALL, 5 );

	m_dChadMyslinsky = new wxStaticText( m_dScroll, wxID_ANY, wxT("Chad Myslinsky"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dChadMyslinsky->Wrap( -1 );
	m_dSizer->Add( m_dChadMyslinsky, 0, wxALL, 5 );

	m_dCharlieBruckner = new wxStaticText( m_dScroll, wxID_ANY, wxT("Charlie Bruckner"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dCharlieBruckner->Wrap( -1 );
	m_dSizer->Add( m_dCharlieBruckner, 0, wxALL, 5 );

	m_dJordanParker = new wxStaticText( m_dScroll, wxID_ANY, wxT("Jordan Parker"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJordanParker->Wrap( -1 );
	m_dSizer->Add( m_dJordanParker, 0, wxALL, 5 );

	m_dRobertChave = new wxStaticText( m_dScroll, wxID_ANY, wxT("Robert Chave Applied Physics Inc"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRobertChave->Wrap( -1 );
	m_dSizer->Add( m_dRobertChave, 0, wxALL, 5 );

	m_dMarvinCalvert = new wxStaticText( m_dScroll, wxID_ANY, wxT("Marvin Calvert"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMarvinCalvert->Wrap( -1 );
	m_dSizer->Add( m_dMarvinCalvert, 0, wxALL, 5 );

	m_dChrisStone = new wxStaticText( m_dScroll, wxID_ANY, wxT("Chris Stone"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dChrisStone->Wrap( -1 );
	m_dSizer->Add( m_dChrisStone, 0, wxALL, 5 );

	m_dErfurterFeurblume = new wxStaticText( m_dScroll, wxID_ANY, wxT("Erfurter Feurblume"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dErfurterFeurblume->Wrap( -1 );
	m_dSizer->Add( m_dErfurterFeurblume, 0, wxALL, 5 );

	m_dMakarenkoAleksey = new wxStaticText( m_dScroll, wxID_ANY, wxT("Makarenko Aleksey"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMakarenkoAleksey->Wrap( -1 );
	m_dSizer->Add( m_dMakarenkoAleksey, 0, wxALL, 5 );

	m_dAnthonyLambiris = new wxStaticText( m_dScroll, wxID_ANY, wxT("Anthony Lambiris"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dAnthonyLambiris->Wrap( -1 );
	m_dSizer->Add( m_dAnthonyLambiris, 0, wxALL, 5 );

	m_dJoeBurtinsky = new wxStaticText( m_dScroll, wxID_ANY, wxT("Joe Burtinsky"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJoeBurtinsky->Wrap( -1 );
	m_dSizer->Add( m_dJoeBurtinsky, 0, wxALL, 5 );

	m_dDalePuckett

	= new wxStaticText( m_dScroll, wxID_ANY, wxT("Dale Puckett"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDalePuckett

	->Wrap( -1 );
	m_dSizer->Add( m_dDalePuckett

	, 0, wxALL, 5 );

	m_dPatrickPreitner = new wxStaticText( m_dScroll, wxID_ANY, wxT("Patrick Preitner"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dPatrickPreitner->Wrap( -1 );
	m_dSizer->Add( m_dPatrickPreitner, 0, wxALL, 5 );

	m_dWilliamSoley = new wxStaticText( m_dScroll, wxID_ANY, wxT("William Soley"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dWilliamSoley->Wrap( -1 );
	m_dSizer->Add( m_dWilliamSoley, 0, wxALL, 5 );

	m_dPhilippRudin = new wxStaticText( m_dScroll, wxID_ANY, wxT("Philipp Rudin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dPhilippRudin->Wrap( -1 );
	m_dSizer->Add( m_dPhilippRudin, 0, wxALL, 5 );

	m_dTerranceWilliams = new wxStaticText( m_dScroll, wxID_ANY, wxT("Terrance Williams"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dTerranceWilliams->Wrap( -1 );
	m_dSizer->Add( m_dTerranceWilliams, 0, wxALL, 5 );

	m_dCharlesSmith = new wxStaticText( m_dScroll, wxID_ANY, wxT("Charles Smith"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dCharlesSmith->Wrap( -1 );
	m_dSizer->Add( m_dCharlesSmith, 0, wxALL, 5 );

	m_dIanBrooks = new wxStaticText( m_dScroll, wxID_ANY, wxT("Ian Brooks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dIanBrooks->Wrap( -1 );
	m_dSizer->Add( m_dIanBrooks, 0, wxALL, 5 );

	m_dIJorgBehrens = new wxStaticText( m_dScroll, wxID_ANY, wxT("Jörg Behrens"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dIJorgBehrens->Wrap( -1 );
	m_dSizer->Add( m_dIJorgBehrens, 0, wxALL, 5 );

	m_dDanielGarley = new wxStaticText( m_dScroll, wxID_ANY, wxT("Daniel Garley"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDanielGarley->Wrap( -1 );
	m_dSizer->Add( m_dDanielGarley, 0, wxALL, 5 );

	m_dDavidWitten = new wxStaticText( m_dScroll, wxID_ANY, wxT("David Witten"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDavidWitten->Wrap( -1 );
	m_dSizer->Add( m_dDavidWitten, 0, wxALL, 5 );

	m_dMartinLanda = new wxStaticText( m_dScroll, wxID_ANY, wxT("Martin Landa"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMartinLanda->Wrap( -1 );
	m_dSizer->Add( m_dMartinLanda, 0, wxALL, 5 );

	m_dJosStark = new wxStaticText( m_dScroll, wxID_ANY, wxT("Jos Stark"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJosStark->Wrap( -1 );
	m_dSizer->Add( m_dJosStark, 0, wxALL, 5 );

	m_dGeoffroyKoechlin = new wxStaticText( m_dScroll, wxID_ANY, wxT("Geoffroy Koechlin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dGeoffroyKoechlin->Wrap( -1 );
	m_dSizer->Add( m_dGeoffroyKoechlin, 0, wxALL, 5 );

	m_dMichalPas = new wxStaticText( m_dScroll, wxID_ANY, wxT("Michal Pas"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMichalPas->Wrap( -1 );
	m_dSizer->Add( m_dMichalPas, 0, wxALL, 5 );

	m_dWillEntriken = new wxStaticText( m_dScroll, wxID_ANY, wxT("Will Entriken"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dWillEntriken->Wrap( -1 );
	m_dSizer->Add( m_dWillEntriken, 0, wxALL, 5 );

	m_dManuelVerdeSalmeron = new wxStaticText( m_dScroll, wxID_ANY, wxT("Manuel Verde Salmeron"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dManuelVerdeSalmeron->Wrap( -1 );
	m_dSizer->Add( m_dManuelVerdeSalmeron, 0, wxALL, 5 );

	m_dJuhaPekkaHoglund = new wxStaticText( m_dScroll, wxID_ANY, wxT("Juha-Pekka Hoglund"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJuhaPekkaHoglund->Wrap( -1 );
	m_dSizer->Add( m_dJuhaPekkaHoglund, 0, wxALL, 5 );

	m_dGoetzStroemsdoerfer = new wxStaticText( m_dScroll, wxID_ANY, wxT("Goetz Stroemsdoerfer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dGoetzStroemsdoerfer->Wrap( -1 );
	m_dSizer->Add( m_dGoetzStroemsdoerfer, 0, wxALL, 5 );

	m_dJohnJBurgessJr = new wxStaticText( m_dScroll, wxID_ANY, wxT("John J Burgess Jr"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJohnJBurgessJr->Wrap( -1 );
	m_dSizer->Add( m_dJohnJBurgessJr, 0, wxALL, 5 );

	m_dArturoCaballero = new wxStaticText( m_dScroll, wxID_ANY, wxT("Arturo Caballero"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dArturoCaballero->Wrap( -1 );
	m_dSizer->Add( m_dArturoCaballero, 0, wxALL, 5 );

	m_dRonaldTissier = new wxStaticText( m_dScroll, wxID_ANY, wxT("Ronald Tissier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRonaldTissier->Wrap( -1 );
	m_dSizer->Add( m_dRonaldTissier, 0, wxALL, 5 );

	m_dDavidLawrence = new wxStaticText( m_dScroll, wxID_ANY, wxT("David Lawrence"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDavidLawrence->Wrap( -1 );
	m_dSizer->Add( m_dDavidLawrence, 0, wxALL, 5 );

	m_dJohnIsella = new wxStaticText( m_dScroll, wxID_ANY, wxT("John Isella"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJohnIsella->Wrap( -1 );
	m_dSizer->Add( m_dJohnIsella, 0, wxALL, 5 );

	m_dChrisKurowicki = new wxStaticText( m_dScroll, wxID_ANY, wxT("Chris Kurowicki"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dChrisKurowicki->Wrap( -1 );
	m_dSizer->Add( m_dChrisKurowicki, 0, wxALL, 5 );

	m_dWilliamCollins = new wxStaticText( m_dScroll, wxID_ANY, wxT("William Collins"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dWilliamCollins->Wrap( -1 );
	m_dSizer->Add( m_dWilliamCollins, 0, wxALL, 5 );

	m_dStefanoPasini = new wxStaticText( m_dScroll, wxID_ANY, wxT("Stefano Pasini"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dStefanoPasini->Wrap( -1 );
	m_dSizer->Add( m_dStefanoPasini, 0, wxALL, 5 );

	m_dScottForbes = new wxStaticText( m_dScroll, wxID_ANY, wxT("Scott Forbes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dScottForbes->Wrap( -1 );
	m_dSizer->Add( m_dScottForbes, 0, wxALL, 5 );

	m_dRafaelMendezTrigueros = new wxStaticText( m_dScroll, wxID_ANY, wxT("Rafael Mendez Trigueros"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dRafaelMendezTrigueros->Wrap( -1 );
	m_dSizer->Add( m_dRafaelMendezTrigueros, 0, wxALL, 5 );

	m_dDuaneDamiano = new wxStaticText( m_dScroll, wxID_ANY, wxT("Duane Damiano"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDuaneDamiano->Wrap( -1 );
	m_dSizer->Add( m_dDuaneDamiano, 0, wxALL, 5 );

	m_dWhitakerAudio = new wxStaticText( m_dScroll, wxID_ANY, wxT("WhitakerAudio"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dWhitakerAudio->Wrap( -1 );
	m_dSizer->Add( m_dWhitakerAudio, 0, wxALL, 5 );

	m_dAlanNashRadiusLLC = new wxStaticText( m_dScroll, wxID_ANY, wxT("Alan at Radius LLC"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dAlanNashRadiusLLC->Wrap( -1 );
	m_dSizer->Add( m_dAlanNashRadiusLLC, 0, wxALL, 5 );

	m_dMelvinCharters = new wxStaticText( m_dScroll, wxID_ANY, wxT("Melvin Charters"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMelvinCharters->Wrap( -1 );
	m_dSizer->Add( m_dMelvinCharters, 0, wxALL, 5 );

	m_dSethFulton = new wxStaticText( m_dScroll, wxID_ANY, wxT("Seth Fulton"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dSethFulton->Wrap( -1 );
	m_dSizer->Add( m_dSethFulton, 0, wxALL, 5 );

	m_dBoydKing = new wxStaticText( m_dScroll, wxID_ANY, wxT("Boyd King"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dBoydKing->Wrap( -1 );
	m_dSizer->Add( m_dBoydKing, 0, wxALL, 5 );

	m_dThomasZicarelli = new wxStaticText( m_dScroll, wxID_ANY, wxT("Thomas Zicarelli"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dThomasZicarelli->Wrap( -1 );
	m_dSizer->Add( m_dThomasZicarelli, 0, wxALL, 5 );

	m_dDarrinAbend = new wxStaticText( m_dScroll, wxID_ANY, wxT("Darrin Abend"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDarrinAbend->Wrap( -1 );
	m_dSizer->Add( m_dDarrinAbend, 0, wxALL, 5 );

	m_dDavidSchulz = new wxStaticText( m_dScroll, wxID_ANY, wxT("David Schulz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDavidSchulz->Wrap( -1 );
	m_dSizer->Add( m_dDavidSchulz, 0, wxALL, 5 );

	m_dKlaudiusGodziek = new wxStaticText( m_dScroll, wxID_ANY, wxT("Klaudius Godziek"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dKlaudiusGodziek->Wrap( -1 );
	m_dSizer->Add( m_dKlaudiusGodziek, 0, wxALL, 5 );

	m_dDonaldDryden = new wxStaticText( m_dScroll, wxID_ANY, wxT("Donald Dryden"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dDonaldDryden->Wrap( -1 );
	m_dSizer->Add( m_dDonaldDryden, 0, wxALL, 5 );

	m_dSDCSistemasdeControl = new wxStaticText( m_dScroll, wxID_ANY, wxT("SDC Sistemas de Control"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dSDCSistemasdeControl->Wrap( -1 );
	m_dSizer->Add( m_dSDCSistemasdeControl, 0, wxALL, 5 );

	m_dVicBaker = new wxStaticText( m_dScroll, wxID_ANY, wxT("Vic Baker"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dVicBaker->Wrap( -1 );
	m_dSizer->Add( m_dVicBaker, 0, wxALL, 5 );

	m_dLuisGimenezCarpena = new wxStaticText( m_dScroll, wxID_ANY, wxT("Luis Gimenez Carpena"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dLuisGimenezCarpena->Wrap( -1 );
	m_dSizer->Add( m_dLuisGimenezCarpena, 0, wxALL, 5 );

	m_dJohnHutchins = new wxStaticText( m_dScroll, wxID_ANY, wxT("John Hutchins"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJohnHutchins->Wrap( -1 );
	m_dSizer->Add( m_dJohnHutchins, 0, wxALL, 5 );

	m_dChrisAlbone = new wxStaticText( m_dScroll, wxID_ANY, wxT("Chris Albone"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dChrisAlbone->Wrap( -1 );
	m_dSizer->Add( m_dChrisAlbone, 0, wxALL, 5 );

	m_dHenriDavid = new wxStaticText( m_dScroll, wxID_ANY, wxT("Henri David"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dHenriDavid->Wrap( -1 );
	m_dSizer->Add( m_dHenriDavid, 0, wxALL, 5 );

	m_dKoyoMasore = new wxStaticText( m_dScroll, wxID_ANY, wxT("Koyo Masore"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dKoyoMasore->Wrap( -1 );
	m_dSizer->Add( m_dKoyoMasore, 0, wxALL, 5 );

	m_dMarkJacob = new wxStaticText( m_dScroll, wxID_ANY, wxT("Mark Jacob"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMarkJacob->Wrap( -1 );
	m_dSizer->Add( m_dMarkJacob, 0, wxALL, 5 );

	m_dLuisAynat = new wxStaticText( m_dScroll, wxID_ANY, wxT("Luis Aynat"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dLuisAynat->Wrap( -1 );
	m_dSizer->Add( m_dLuisAynat, 0, wxALL, 5 );

	m_dMichaelCarter = new wxStaticText( m_dScroll, wxID_ANY, wxT("Michael Carter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dMichaelCarter->Wrap( -1 );
	m_dSizer->Add( m_dMichaelCarter, 0, wxALL, 5 );

	m_dManuelSerranoCardenas = new wxStaticText( m_dScroll, wxID_ANY, wxT("Manuel Serrano Cárdenas"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dManuelSerranoCardenas->Wrap( -1 );
	m_dSizer->Add( m_dManuelSerranoCardenas, 0, wxALL, 5 );

	m_dVincentPortier = new wxStaticText( m_dScroll, wxID_ANY, wxT("Vincent Portier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dVincentPortier->Wrap( -1 );
	m_dSizer->Add( m_dVincentPortier, 0, wxALL, 5 );

	m_dJeanFrancoisLaperriere = new wxStaticText( m_dScroll, wxID_ANY, wxT("Jean-Francois Laperriere"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dJeanFrancoisLaperriere->Wrap( -1 );
	m_dSizer->Add( m_dJeanFrancoisLaperriere, 0, wxALL, 5 );


	m_dScroll->SetSizer( m_dSizer );
	m_dScroll->Layout();
	m_dSizer->Fit( m_dScroll );
	m_dScrollSizer->Add( m_dScroll, 1, wxEXPAND | wxALL, 5 );


	m_dPanel->SetSizer( m_dScrollSizer );
	m_dPanel->Layout();
	m_dScrollSizer->Fit( m_dPanel );
	m_aboutNotebook->AddPage( m_dPanel, wxT("Donations"), false );
	m_stPanel = new wxPanel( m_aboutNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_stScrollSizer;
	m_stScrollSizer = new wxBoxSizer( wxVERTICAL );

	m_stScroll = new wxScrolledWindow( m_stPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_stScroll->SetScrollRate( 5, 5 );
	wxBoxSizer* m_stBSizer;
	m_stBSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* m_stSizer;
	m_stSizer = new wxBoxSizer( wxVERTICAL );

	m_stHeader = new wxStaticText( m_stScroll, wxID_ANY, wxT("Special Thanks To"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_stHeader->Wrap( -1 );
	m_stHeader->SetFont( wxFont( 15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_stSizer->Add( m_stHeader, 0, wxALL, 5 );

	m_stDivider1 = new wxStaticLine( m_stScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_stSizer->Add( m_stDivider1, 0, wxEXPAND | wxALL, 5 );

	m_stSoapyDevAssistHeader = new wxStaticText( m_stScroll, wxID_ANY, wxT("SoapySDR Development and Assistance:"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_stSoapyDevAssistHeader->Wrap( -1 );
	m_stSoapyDevAssistHeader->SetFont( wxFont( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_stSizer->Add( m_stSoapyDevAssistHeader, 0, wxALL, 5 );

	m_stJoshBlum = new wxStaticText( m_stScroll, wxID_ANY, wxT("Josh Blum / @guruofquality / pothosware.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stJoshBlum->Wrap( -1 );
	m_stSizer->Add( m_stJoshBlum, 0, wxALL, 5 );

	m_stDivider2 = new wxStaticLine( m_stScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_stSizer->Add( m_stDivider2, 0, wxEXPAND | wxALL, 5 );

	m_stLiquidDSPHeader = new wxStaticText( m_stScroll, wxID_ANY, wxT("Liquid-DSP Development and Assistance:"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_stLiquidDSPHeader->Wrap( -1 );
	m_stLiquidDSPHeader->SetFont( wxFont( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_stSizer->Add( m_stLiquidDSPHeader, 0, wxALL, 5 );

	m_stJosephGaeddert = new wxStaticText( m_stScroll, wxID_ANY, wxT("Joseph D. Gaeddert / @jgaeddert / liquidsdr.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stJosephGaeddert->Wrap( -1 );
	m_stSizer->Add( m_stJosephGaeddert, 0, wxALL, 5 );

	m_stDivider3 = new wxStaticLine( m_stScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_stSizer->Add( m_stDivider3, 0, wxEXPAND | wxALL, 5 );

	m_stIdeasDirectionsHeader = new wxStaticText( m_stScroll, wxID_ANY, wxT("Ideas, Direction && Encouragement:"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_stIdeasDirectionsHeader->Wrap( -1 );
	m_stIdeasDirectionsHeader->SetFont( wxFont( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_stSizer->Add( m_stIdeasDirectionsHeader, 0, wxALL, 5 );

	m_stTonMachielsen = new wxStaticText( m_stScroll, wxID_ANY, wxT("Ton Machielsen / @Toontje / @EA3HOE "), wxDefaultPosition, wxDefaultSize, 0 );
	m_stTonMachielsen->Wrap( -1 );
	m_stSizer->Add( m_stTonMachielsen, 0, wxALL, 5 );

	m_stMikeLadd = new wxStaticText( m_stScroll, wxID_ANY, wxT("Mike Ladd / KD2KOG.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMikeLadd->Wrap( -1 );
	m_stSizer->Add( m_stMikeLadd, 0, wxALL, 5 );

	m_stSDRplay = new wxStaticText( m_stScroll, wxID_ANY, wxT("SDRplay team / @SDRplay / SDRplay.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSDRplay->Wrap( -1 );
	m_stSizer->Add( m_stSDRplay, 0, wxALL, 5 );

	m_stSDRplayFB = new wxStaticText( m_stScroll, wxID_ANY, wxT("SDRplay Facebook group"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSDRplayFB->Wrap( -1 );
	m_stSizer->Add( m_stSDRplayFB, 0, wxALL, 5 );

	m_stPaulWarren = new wxStaticText( m_stScroll, wxID_ANY, wxT("Paul Warren / @pwarren"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPaulWarren->Wrap( -1 );
	m_stSizer->Add( m_stPaulWarren, 0, wxALL, 5 );

	m_stSegesdiKaroly = new wxStaticText( m_stScroll, wxID_ANY, wxT("Segesdi Károly / @jazzkutya"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stSegesdiKaroly->Wrap( -1 );
	m_stSizer->Add( m_stSegesdiKaroly, 0, wxALL, 5 );

	m_stRedditRTLSDR = new wxStaticText( m_stScroll, wxID_ANY, wxT("Reddit RTL-SDR group /r/rtlsdr"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stRedditRTLSDR->Wrap( -1 );
	m_stSizer->Add( m_stRedditRTLSDR, 0, wxALL, 5 );

	m_stNooElec = new wxStaticText( m_stScroll, wxID_ANY, wxT("NooElec team / NooElec.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stNooElec->Wrap( -1 );
	m_stSizer->Add( m_stNooElec, 0, wxALL, 5 );

	m_stGHIssues = new wxStaticText( m_stScroll, wxID_ANY, wxT("Everyone who's contributed to the GitHub issues; thanks!"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stGHIssues->Wrap( -1 );
	m_stSizer->Add( m_stGHIssues, 0, wxALL, 5 );

	m_stNominate = new wxStaticText( m_stScroll, wxID_ANY, wxT("Please feel free to nominate anyone we might have missed."), wxDefaultPosition, wxDefaultSize, 0 );
	m_stNominate->Wrap( -1 );
	m_stSizer->Add( m_stNominate, 0, wxALL, 5 );


	m_stBSizer->Add( m_stSizer, 1, wxALL|wxEXPAND, 5 );


	m_stScroll->SetSizer( m_stBSizer );
	m_stScroll->Layout();
	m_stBSizer->Fit( m_stScroll );
	m_stScrollSizer->Add( m_stScroll, 1, wxEXPAND | wxALL, 5 );


	m_stPanel->SetSizer( m_stScrollSizer );
	m_stPanel->Layout();
	m_stScrollSizer->Fit( m_stPanel );
	m_aboutNotebook->AddPage( m_stPanel, wxT("Special Thanks"), false );

	dlgSizer->Add( m_aboutNotebook, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( dlgSizer );
	this->Layout();

	this->Centre( wxBOTH );
}

AboutDialogBase::~AboutDialogBase()
{
}
