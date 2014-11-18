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


class DemodulatorThread: public wxThread {
public:
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

    unsigned int wbfm_frequency;
    msresamp_crcf wbfm_resampler;
    float wbfm_resample_ratio;

    unsigned int audio_frequency;
    msresamp_crcf audio_resampler;
    float audio_resample_ratio;

    freqdem fdem;
};
