///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 23 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __ABOUTDIALOGBASE_H__
#define __ABOUTDIALOGBASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/scrolwin.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class AboutDialogBase
///////////////////////////////////////////////////////////////////////////////
class AboutDialogBase : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_hPanel;
		wxStaticText* m_appName;
		wxNotebook* m_aboutNotebook;
		wxScrolledWindow* m_dbScroll;
		wxStaticText* m_dbHeader;
		wxStaticText* m_dbGHHeader;
		wxStaticText* m_dbTwitter;
		wxStaticText* m_dbCharlesCliffe;
		wxStaticText* m_dbghCC;
		wxStaticText* m_dbtCC;
		wxStaticText* m_dbVincentSonnier;
		wxStaticText* m_dbghVS;
		wxStaticText* m_dbtVS;
		wxStaticLine* m_dbDivider1;
		wxStaticText* m_cContributorsHeader;
		wxStaticText* m_cGitHub;
		wxStaticText* m_cCorneLukken;
		wxStaticText* m_cghCL;
		wxStaticText* m_cStainislawPitucha;
		wxStaticText* m_cghSP;
		wxStaticText* m_cghStefanTalpalaru;
		wxStaticText* m_cghST;
		wxStaticText* m_cCrisMotch;
		wxStaticText* m_cghCM;
		wxStaticText* m_cMariuszRyndzionek;
		wxStaticText* m_cghMR;
		wxStaticText* m_cJiangWei;
		wxStaticText* m_cghJW;
		wxStaticText* m_cTomSwartz;
		wxStaticText* m_cghTS;
		wxStaticText* m_cInfinityCyberworks;
		wxStaticText* m_cghIC;
		wxScrolledWindow* m_dScroll;
		wxStaticText* m_dHeader;
		wxStaticLine* m_dDivider1;
		wxStaticText* m_dSDRplay;
		wxStaticText* m_dMichaelLadd;
		wxStaticText* m_dAutoMotiveTemplates;
		wxStaticText* m_dJorgeMorales;
		wxStaticText* m_dMichaelRooke;
		wxStaticText* m_dTNCOM;
		wxStaticText* m_dErikWied;
		wxStaticText* m_dRobertDuering;
		wxStaticText* m_dJimDeitch;
		wxStaticText* m_dNooElec;
		wxStaticText* m_dDavidAhlgren;
		wxStaticText* m_dRonaldCook;
		wxStaticText* m_dEricPeterson;
		wxStaticText* m_dGeoDistributing;
		wxStaticText* m_dJamesCarson;
		wxStaticText* m_dCraigWilliams;
		wxStaticText* m_dRudolfShaffer;
		wxStaticText* m_dJohnKaton;
		wxStaticText* m_dVincentSonnier;
		wxStaticText* m_dCorq;
		wxStaticText* m_dIvanAlekseev;
		wxStaticText* m_dOleJorgenKolsrud;
		wxStaticText* m_dHenrikJagemyr;
		wxStaticText* m_dPeterHaines;
		wxStaticText* m_dLeonAbrassart;
		wxStaticText* m_dGeorgeTalbot;
		wxStaticText* m_dFranciscoPuerta;
		wxStaticText* m_dRonaldLundeen;
		wxStaticText* m_dWalterHorbert;
		wxStaticText* m_dWilliamLD;
		wxStaticText* m_dBratislavArandjelovic;
		wxStaticText* m_dGaryMartin;
		wxStaticText* m_dEinarsRepse;
		wxStaticText* m_dTimothyGatton;
		wxStaticText* m_dStephenCuccio;
		wxStaticText* m_dKeshavlalPatel;
		wxScrolledWindow* m_stScroll;
		wxStaticText* m_stHeader;
		wxStaticLine* m_stDivider1;
		wxStaticText* m_stSoapyDevAssistHeader;
		wxStaticText* m_stJoshBlum;
		wxStaticLine* m_stDivider2;
		wxStaticText* m_stLiquidDSPHeader;
		wxStaticText* m_stJosephGaeddert;
		wxStaticLine* m_stDivider3;
		wxStaticText* m_stIdeasDirectionsHeader;
		wxStaticText* m_stTonMachielsen;
		wxStaticText* m_stMikeLadd;
		wxStaticText* m_stSDRplay;
		wxStaticText* m_stSDRplayFB;
		wxStaticText* m_stPaulWarren;
		wxStaticText* m_stSegesdiKaroly;
		wxStaticText* m_stRedditRTLSDR;
		wxStaticText* m_stNooElec;
		wxStaticText* m_stGHIssues;
		wxStaticText* m_stNominate;
	
	public:
		
		AboutDialogBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("About"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 436,378 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~AboutDialogBase();
	
};

#endif //__ABOUTDIALOGBASE_H__
