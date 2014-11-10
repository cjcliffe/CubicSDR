#pragma once

#include "wx/wxprec.h"
#include "rtl-sdr.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "SDRThread.h"
#include "IQBufferThread.h"
#include "SDRThreadQueue.h"

// declare a new type of event, to be used by our SDRThread class:
//wxDECLARE_EVENT(wxEVT_COMMAND_SDRThread_COMPLETED, wxThreadEvent);
//wxDECLARE_EVENT(wxEVT_COMMAND_SDRThread_UPDATE, wxThreadEvent);
//wxDECLARE_EVENT(wxEVT_COMMAND_SDRThread_INPUT, wxThreadEvent);

enum {
    EVENT_SDR_INPUT = wxID_HIGHEST + 1
};

class SDRThread: public wxThread {
public:
    rtlsdr_dev_t *dev;

    SDRThread(SDRThreadQueue* pQueue, int id = 0);
    ~SDRThread();

    int enumerate_rtl();

protected:
    virtual ExitCode Entry();
    uint32_t sample_rate;
    SDRThreadQueue* m_pQueue;
    int m_ID;
};
