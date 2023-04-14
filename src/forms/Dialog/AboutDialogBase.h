///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

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
		wxPanel* m_dbPanel;
		wxScrolledWindow* m_dbScroll;
		wxPanel* m_dbPanel1;
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
		wxPanel* m_dbPanel2;
		wxStaticText* m_cContributorsHeader;
		wxStaticText* m_cGitHub;
		wxStaticText* m_cCorneLukken;
		wxStaticText* m_cghCL;
		wxStaticText* m_cStainislawPitucha;
		wxStaticText* m_cghSP;
		wxStaticText* m_cTomSwartz;
		wxStaticText* m_cghTS;
		wxStaticText* m_cStefanTalpalaru;
		wxStaticText* m_cghST;
		wxStaticText* m_cDellRaySackett;
		wxStaticText* m_cghDRS;
		wxStaticText* m_cJiangWei;
		wxStaticText* m_cghJW;
		wxStaticText* m_cInfinityCyberworks;
		wxStaticText* m_cghIC;
		wxStaticText* m_cCrisMotch;
		wxStaticText* m_cghCM;
		wxStaticText* m_cAntiHax;
		wxStaticText* m_cghAH;
		wxStaticText* m_cRainbow;
		wxStaticText* m_cghRBW;
		wxStaticText* m_cMariuszRyndzionek;
		wxStaticText* m_cghMR;
		wxStaticText* m_cDrahosj;
		wxStaticText* m_cghDra;
		wxStaticText* m_cBenoitAllard;
		wxStaticText* m_cghBA;
		wxStaticText* m_cDianeBruce;
		wxStaticText* m_cghDiB;
		wxStaticText* m_cPaulColby;
		wxStaticText* m_cghCACRI;
		wxStaticText* m_cclassabbyamp;
		wxStaticText* m_cghclassabbyamp;
		wxStaticText* m_cSDRplay;
		wxStaticText* m_cghSDRplay;
		wxStaticText* m_cdforsi;
		wxStaticText* m_cghdforsi;
		wxStaticText* m_cMagalex2x14;
		wxStaticText* m_cghMagalex2x14;
		wxStaticText* m_cjawatson;
		wxStaticText* m_cghjawatson;
		wxStaticText* m_cghf4grx;
		wxStaticText* m_cf4grx;
		wxPanel* m_dPanel;
		wxScrolledWindow* m_dScroll;
		wxStaticText* m_dHeader;
		wxStaticLine* m_dDivider1;
		wxStaticText* m_dSDRplay;
		wxStaticText* m_dMichaelLadd;
		wxStaticText* m_dAutoMotiveTemplates;
		wxStaticText* m_dJorgeMorales;
		wxStaticText* m_dMichaelRooke;
		wxStaticText* m_dErikWied;
		wxStaticText* m_dChristopherEsser;
		wxStaticText* m_dTNCOM;
		wxStaticText* m_dAlexanderSadleir;
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
		wxStaticText* m_dBobSchatzman;
		wxStaticText* m_dRobertRoss;
		wxStaticText* m_dRobertoBellotti;
		wxStaticText* m_dSergeVanderTorre;
		wxStaticText* m_dDieterSchneider;
		wxStaticText* m_dPetrikaJaneku;
		wxStaticText* m_dChadMyslinsky;
		wxStaticText* m_dCharlieBruckner;
		wxStaticText* m_dJordanParker;
		wxStaticText* m_dRobertChave;
		wxStaticText* m_dMarvinCalvert;
		wxStaticText* m_dChrisStone;
		wxStaticText* m_dErfurterFeurblume;
		wxStaticText* m_dMakarenkoAleksey;
		wxStaticText* m_dAnthonyLambiris;
		wxStaticText* m_dJoeBurtinsky;
		wxStaticText* m_dDalePuckett

		;
		wxStaticText* m_dPatrickPreitner;
		wxStaticText* m_dWilliamSoley;
		wxStaticText* m_dPhilippRudin;
		wxStaticText* m_dTerranceWilliams;
		wxStaticText* m_dCharlesSmith;
		wxStaticText* m_dIanBrooks;
		wxStaticText* m_dIJorgBehrens;
		wxStaticText* m_dDanielGarley;
		wxStaticText* m_dDavidWitten;
		wxStaticText* m_dMartinLanda;
		wxStaticText* m_dJosStark;
		wxStaticText* m_dGeoffroyKoechlin;
		wxStaticText* m_dMichalPas;
		wxStaticText* m_dWillEntriken;
		wxStaticText* m_dManuelVerdeSalmeron;
		wxStaticText* m_dJuhaPekkaHoglund;
		wxStaticText* m_dGoetzStroemsdoerfer;
		wxStaticText* m_dJohnJBurgessJr;
		wxStaticText* m_dArturoCaballero;
		wxStaticText* m_dRonaldTissier;
		wxStaticText* m_dDavidLawrence;
		wxStaticText* m_dJohnIsella;
		wxStaticText* m_dChrisKurowicki;
		wxStaticText* m_dWilliamCollins;
		wxStaticText* m_dStefanoPasini;
		wxStaticText* m_dScottForbes;
		wxStaticText* m_dRafaelMendezTrigueros;
		wxStaticText* m_dDuaneDamiano;
		wxStaticText* m_dWhitakerAudio;
		wxStaticText* m_dAlanNashRadiusLLC;
		wxStaticText* m_dMelvinCharters;
		wxStaticText* m_dSethFulton;
		wxStaticText* m_dBoydKing;
		wxStaticText* m_dThomasZicarelli;
		wxStaticText* m_dDarrinAbend;
		wxStaticText* m_dDavidSchulz;
		wxStaticText* m_dKlaudiusGodziek;
		wxStaticText* m_dDonaldDryden;
		wxStaticText* m_dSDCSistemasdeControl;
		wxStaticText* m_dVicBaker;
		wxStaticText* m_dLuisGimenezCarpena;
		wxStaticText* m_dJohnHutchins;
		wxStaticText* m_dChrisAlbone;
		wxStaticText* m_dHenriDavid;
		wxStaticText* m_dKoyoMasore;
		wxStaticText* m_dMarkJacob;
		wxStaticText* m_dLuisAynat;
		wxStaticText* m_dMichaelCarter;
		wxStaticText* m_dManuelSerranoCardenas;
		wxStaticText* m_dVincentPortier;
		wxStaticText* m_dJeanFrancoisLaperriere;
		wxPanel* m_stPanel;
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

		AboutDialogBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("About"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 600,450 ), long style = wxDEFAULT_DIALOG_STYLE );

		~AboutDialogBase();

};

