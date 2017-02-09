///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 23 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
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
	
	m_appName = new wxStaticText( m_hPanel, wxID_ANY, wxT("CubicSDR"), wxDefaultPosition, wxDefaultSize, 0 );
	m_appName->Wrap( -1 );
	m_appName->SetFont( wxFont( 20, 70, 90, 90, false, wxEmptyString ) );
	
	m_hSizer->Add( m_appName, 0, wxALL, 6 );
	
	
	m_hPanel->SetSizer( m_hSizer );
	m_hPanel->Layout();
	m_hSizer->Fit( m_hPanel );
	dlgSizer->Add( m_hPanel, 0, wxALL|wxEXPAND, 5 );
	
	m_aboutNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_dbScroll = new wxScrolledWindow( m_aboutNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_dbScroll->SetScrollRate( 5, 5 );
	wxBoxSizer* m_dbPane;
	m_dbPane = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* m_dbSizer;
	m_dbSizer = new wxFlexGridSizer( 0, 3, 2, 20 );
	m_dbSizer->SetFlexibleDirection( wxBOTH );
	m_dbSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	m_dbHeader = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Developed By"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_dbHeader->Wrap( -1 );
	m_dbHeader->SetFont( wxFont( 15, 70, 90, 90, false, wxEmptyString ) );
	
	m_dbSizer->Add( m_dbHeader, 0, wxALL, 5 );
	
	m_dbGHHeader = new wxStaticText( m_dbScroll, wxID_ANY, wxT("GitHub"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbGHHeader->Wrap( -1 );
	m_dbGHHeader->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	m_dbSizer->Add( m_dbGHHeader, 0, wxALL, 5 );
	
	m_dbTwitter = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Twitter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbTwitter->Wrap( -1 );
	m_dbTwitter->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	m_dbSizer->Add( m_dbTwitter, 0, wxALL, 5 );
	
	m_dbCharlesCliffe = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Charles J. Cliffe"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbCharlesCliffe->Wrap( -1 );
	m_dbSizer->Add( m_dbCharlesCliffe, 0, wxALL, 5 );
	
	m_dbghCC = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@cjcliffe"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbghCC->Wrap( -1 );
	m_dbSizer->Add( m_dbghCC, 0, wxALL, 5 );
	
	m_dbtCC = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@ccliffe"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbtCC->Wrap( -1 );
	m_dbSizer->Add( m_dbtCC, 0, wxALL, 5 );
	
	m_dbVincentSonnier = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Vincent Sonnier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbVincentSonnier->Wrap( -1 );
	m_dbSizer->Add( m_dbVincentSonnier, 0, wxALL, 5 );
	
	m_dbghVS = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@vsonnier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbghVS->Wrap( -1 );
	m_dbSizer->Add( m_dbghVS, 0, wxALL, 5 );
	
	m_dbtVS = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@VincentSonnier"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dbtVS->Wrap( -1 );
	m_dbSizer->Add( m_dbtVS, 0, wxALL, 5 );
	
	
	m_dbPane->Add( m_dbSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_dbDivider1 = new wxStaticLine( m_dbScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_dbPane->Add( m_dbDivider1, 0, wxALL|wxEXPAND, 10 );
	
	wxFlexGridSizer* m_cSizer;
	m_cSizer = new wxFlexGridSizer( 0, 2, 2, 20 );
	m_cSizer->SetFlexibleDirection( wxBOTH );
	m_cSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	
	m_cContributorsHeader = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Contributors"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_cContributorsHeader->Wrap( -1 );
	m_cContributorsHeader->SetFont( wxFont( 15, 70, 90, 90, false, wxEmptyString ) );
	
	m_cSizer->Add( m_cContributorsHeader, 0, wxALL, 5 );
	
	m_cGitHub = new wxStaticText( m_dbScroll, wxID_ANY, wxT("GitHub"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cGitHub->Wrap( -1 );
	m_cGitHub->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );
	
	m_cSizer->Add( m_cGitHub, 0, wxALL, 5 );
	
	m_cCorneLukken = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Corne Lukken"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cCorneLukken->Wrap( -1 );
	m_cSizer->Add( m_cCorneLukken, 0, wxALL, 5 );
	
	m_cghCL = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@Dantali0n"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghCL->Wrap( -1 );
	m_cSizer->Add( m_cghCL, 0, wxALL, 5 );
	
	m_cStainislawPitucha = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Stanisław Pitucha"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cStainislawPitucha->Wrap( -1 );
	m_cSizer->Add( m_cStainislawPitucha, 0, wxALL, 5 );
	
	m_cghSP = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@viraptor"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghSP->Wrap( -1 );
	m_cSizer->Add( m_cghSP, 0, wxALL, 5 );
	
	m_cghStefanTalpalaru = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Ștefan Talpalaru"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghStefanTalpalaru->Wrap( -1 );
	m_cSizer->Add( m_cghStefanTalpalaru, 0, wxALL, 5 );
	
	m_cghST = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@stefantalpalaru"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghST->Wrap( -1 );
	m_cSizer->Add( m_cghST, 0, wxALL, 5 );
	
	m_cCrisMotch = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Chris Motch"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cCrisMotch->Wrap( -1 );
	m_cSizer->Add( m_cCrisMotch, 0, wxALL, 5 );
	
	m_cghCM = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@bodrick"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghCM->Wrap( -1 );
	m_cSizer->Add( m_cghCM, 0, wxALL, 5 );
	
	m_cMariuszRyndzionek = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Mariusz Ryndzionek"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cMariuszRyndzionek->Wrap( -1 );
	m_cSizer->Add( m_cMariuszRyndzionek, 0, wxALL, 5 );
	
	m_cghMR = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@mryndzionek"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghMR->Wrap( -1 );
	m_cSizer->Add( m_cghMR, 0, wxALL, 5 );
	
	m_cJiangWei = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Jiang Wei"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cJiangWei->Wrap( -1 );
	m_cSizer->Add( m_cJiangWei, 0, wxALL, 5 );
	
	m_cghJW = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@jocover"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghJW->Wrap( -1 );
	m_cSizer->Add( m_cghJW, 0, wxALL, 5 );
	
	m_cTomSwartz = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Tom Swartz"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cTomSwartz->Wrap( -1 );
	m_cSizer->Add( m_cTomSwartz, 0, wxALL, 5 );
	
	m_cghTS = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@tomswartz07"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghTS->Wrap( -1 );
	m_cSizer->Add( m_cghTS, 0, wxALL, 5 );
	
	m_cInfinityCyberworks = new wxStaticText( m_dbScroll, wxID_ANY, wxT("Infinity Cyberworks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cInfinityCyberworks->Wrap( -1 );
	m_cSizer->Add( m_cInfinityCyberworks, 0, wxALL, 5 );
	
	m_cghIC = new wxStaticText( m_dbScroll, wxID_ANY, wxT("@infinitycyberworks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cghIC->Wrap( -1 );
	m_cSizer->Add( m_cghIC, 0, wxALL, 5 );
	
	
	m_dbPane->Add( m_cSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	m_dbScroll->SetSizer( m_dbPane );
	m_dbScroll->Layout();
	m_dbPane->Fit( m_dbScroll );
	m_aboutNotebook->AddPage( m_dbScroll, wxT("Developers"), false );
	m_dScroll = new wxScrolledWindow( m_aboutNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_dScroll->SetScrollRate( 5, 5 );
	wxBoxSizer* m_dBSizer;
	m_dBSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_dSizer;
	m_dSizer = new wxBoxSizer( wxVERTICAL );
	
	m_dHeader = new wxStaticText( m_dScroll, wxID_ANY, wxT("Thanks to everyone who donated at cubicsdr.com!"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_dHeader->Wrap( -1 );
	m_dHeader->SetFont( wxFont( 15, 70, 90, 90, false, wxEmptyString ) );
	
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
	
	m_dTNCOM = new wxStaticText( m_dScroll, wxID_ANY, wxT("TNCOM"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dTNCOM->Wrap( -1 );
	m_dSizer->Add( m_dTNCOM, 0, wxALL, 5 );
	
	m_dErikWied = new wxStaticText( m_dScroll, wxID_ANY, wxT("Erik Mikkel Wied"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dErikWied->Wrap( -1 );
	m_dSizer->Add( m_dErikWied, 0, wxALL, 5 );
	
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
	
	m_dCorq = new wxStaticText( m_dScroll, wxID_ANY, wxT("corq's auctions/L. Easterly LTD (x 4)"), wxDefaultPosition, wxDefaultSize, 0 );
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
	
	
	m_dBSizer->Add( m_dSizer, 1, wxALL|wxEXPAND, 5 );
	
	
	m_dScroll->SetSizer( m_dBSizer );
	m_dScroll->Layout();
	m_dBSizer->Fit( m_dScroll );
	m_aboutNotebook->AddPage( m_dScroll, wxT("Donations"), false );
	m_stScroll = new wxScrolledWindow( m_aboutNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_stScroll->SetScrollRate( 5, 5 );
	wxBoxSizer* m_stBSizer;
	m_stBSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_stSizer;
	m_stSizer = new wxBoxSizer( wxVERTICAL );
	
	m_stHeader = new wxStaticText( m_stScroll, wxID_ANY, wxT("Special Thanks To"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_stHeader->Wrap( -1 );
	m_stHeader->SetFont( wxFont( 15, 70, 90, 90, false, wxEmptyString ) );
	
	m_stSizer->Add( m_stHeader, 0, wxALL, 5 );
	
	m_stDivider1 = new wxStaticLine( m_stScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_stSizer->Add( m_stDivider1, 0, wxEXPAND | wxALL, 5 );
	
	m_stSoapyDevAssistHeader = new wxStaticText( m_stScroll, wxID_ANY, wxT("SoapySDR Development and Assistance:"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_stSoapyDevAssistHeader->Wrap( -1 );
	m_stSoapyDevAssistHeader->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	m_stSizer->Add( m_stSoapyDevAssistHeader, 0, wxALL, 5 );
	
	m_stJoshBlum = new wxStaticText( m_stScroll, wxID_ANY, wxT("Josh Blum / @guruofquality / pothosware.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stJoshBlum->Wrap( -1 );
	m_stSizer->Add( m_stJoshBlum, 0, wxALL, 5 );
	
	m_stDivider2 = new wxStaticLine( m_stScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_stSizer->Add( m_stDivider2, 0, wxEXPAND | wxALL, 5 );
	
	m_stLiquidDSPHeader = new wxStaticText( m_stScroll, wxID_ANY, wxT("Liquid-DSP Development and Assistance:"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_stLiquidDSPHeader->Wrap( -1 );
	m_stLiquidDSPHeader->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
	m_stSizer->Add( m_stLiquidDSPHeader, 0, wxALL, 5 );
	
	m_stJosephGaeddert = new wxStaticText( m_stScroll, wxID_ANY, wxT("Joseph D. Gaeddert / @jgaeddert / liquidsdr.com"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stJosephGaeddert->Wrap( -1 );
	m_stSizer->Add( m_stJosephGaeddert, 0, wxALL, 5 );
	
	m_stDivider3 = new wxStaticLine( m_stScroll, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_stSizer->Add( m_stDivider3, 0, wxEXPAND | wxALL, 5 );
	
	m_stIdeasDirectionsHeader = new wxStaticText( m_stScroll, wxID_ANY, wxT("Ideas, Direction && Encouragement:"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
	m_stIdeasDirectionsHeader->Wrap( -1 );
	m_stIdeasDirectionsHeader->SetFont( wxFont( 10, 70, 90, 92, false, wxEmptyString ) );
	
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
	m_aboutNotebook->AddPage( m_stScroll, wxT("Special Thanks"), false );
	
	dlgSizer->Add( m_aboutNotebook, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( dlgSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

AboutDialogBase::~AboutDialogBase()
{
}
