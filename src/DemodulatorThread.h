#pragma once

#include <queue>
#include <vector>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "DemodulatorThreadQueue.h"

#include "liquid/liquid.h"

// declare a new type of event, to be used by our DemodulatorThread class:
//wxDECLARE_EVENT(wxEVT_COMMAND_DemodulatorThread_COMPLETED, wxThreadEvent);
//wxDECLARE_EVENT(wxEVT_COMMAND_DemodulatorThread_UPDATE, wxThreadEvent);
//wxDECLARE_EVENT(wxEVT_COMMAND_DemodulatorThread_INPUT, wxThreadEvent);

enum {
    EVENT_DEMOD_INPUT = wxID_HIGHEST + 1
};

class DemodulatorThread: public wxThread {
public:
    std::queue<std::vector<float> *> audio_queue;
    unsigned int audio_queue_ptr;

    DemodulatorThread(DemodulatorThreadQueue* pQueue, int id = 0);
    ~DemodulatorThread();

protected:
    virtual ExitCode Entry();
    DemodulatorThreadQueue* m_pQueue;
    int m_ID;

    firfilt_crcf fir_filter;
    firfilt_crcf fir_audio_filter;

    unsigned int bandwidth;

    msresamp_crcf resampler;
    float resample_ratio;

    msresamp_crcf wbfm_resampler;
    float wbfm_resample_ratio;
    unsigned int wbfm_frequency;

    msresamp_crcf audio_resampler;
    float audio_resample_ratio;

    unsigned int audio_frequency;

    freqdem fdem;
};
