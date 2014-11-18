#pragma once

#include "wx/wxprec.h"
#include "rtl-sdr.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "SDRThreadQueue.h"
#include "DemodulatorThreadQueue.h"
#include "DemodulatorMgr.h"

class SDRThread: public wxThread {
public:
    rtlsdr_dev_t *dev;

    SDRThread(SDRThreadQueue* pQueue, int id = 0);
    ~SDRThread();

    int enumerate_rtl();

    void bindDemodulator(DemodulatorInstance &demod) {
        demodulators.push_back(demod.threadQueueDemod);
    }

protected:
    virtual ExitCode Entry();
    uint32_t sample_rate;
    SDRThreadQueue* m_pQueue;
    int m_ID;

    std::vector<DemodulatorThreadQueue *> demodulators;
};
