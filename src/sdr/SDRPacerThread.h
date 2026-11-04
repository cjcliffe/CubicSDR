// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <atomic>
#include <deque>

#include "IOThread.h"
#include "sdr/SDRPacer.h"
#include "sdr/SoapySDRThread.h"

// Steady-cadence stage between SDRThread and SDRPostThread: whole IQ
// blocks in, at most one frame-period worth out per tick. Bypass mode
// forwards everything immediately for the pacing toggle.
class SDRPacerThread : public IOThread {
public:
    SDRPacerThread();

    void run() override;
    void terminate() override;

    void setPacingEnabled(bool enabled);
    bool isPacingEnabled() const;

    void requestFlush();

private:
    SDRPacer pacer;
    std::atomic_bool pacingEnabled;
    std::atomic_bool flushRequested;
    std::deque<SDRThreadIQDataPtr> backlog;
};
