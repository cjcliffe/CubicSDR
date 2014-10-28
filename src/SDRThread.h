#pragma once

#include "wx/wxprec.h"
#include "rtl-sdr.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

// declare a new type of event, to be used by our SDRThread class:
wxDECLARE_EVENT(wxEVT_COMMAND_SDRThread_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_SDRThread_UPDATE, wxThreadEvent);


class SDRThread: public wxThread {
public:
	rtlsdr_dev_t *dev;

	SDRThread(wxApp *app);
	~SDRThread();

	void enumerate_rtl();

protected:
	virtual ExitCode Entry();
	wxApp *handler;
};
